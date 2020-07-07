#include "FlowControlManager.h"
#include "FlowControl.h"

FlowControlManager::FlowControlManager()
{
    _latestDBUpdateTime = 0;
    _maxFlowID = 0;
    _onoff = false;
    //_index = 0;
}

bool FlowControlManager::init(TC_Config &conf)
{
    _dbConf = conf.getDomainMap("/main/db");
    TC_DBConf tcDBConf;
    tcDBConf.loadFromMap(_dbConf);
    _mysql.init(tcDBConf);
    _reportObj = conf.get("/main<flow_report_obj>", "");
    _localIp = conf.get("/main<local_ip>", "");
    if (_localIp.empty())
    {
        _localIp = ServerConfig::LocalIp;
    }
    _onoff = (conf.get("/main<flow_control_onoff>", "on") == "on" ? true : false);

    if (_onoff)
    {
        loadDB();
    }

    return true;
}

bool FlowControlManager::loadDB()
{
    try
    {
        if (_latestDBUpdateTime > 0)
        {
            string sqlCount = "select  max(f_id) as f_id from t_flow_control where f_update_time >= " + TC_Common::tostr(_latestDBUpdateTime);
            TC_Mysql::MysqlData data = _mysql.queryRecord(sqlCount);
            if (data.size() > 0)
            {
                int maxID = TC_Common::strto<int>(data[0]["f_id"]);
                if (maxID == _maxFlowID)
                {
                    return true;
                }
                else
                {
                    TLOGDEBUG(sqlCount << ", maxID:" << maxID << ", last id:" << _maxFlowID << endl);
                }
            }
            else
            {
                TLOGERROR(sqlCount << " ===> no result." << endl);
                return false;
            }
        }

        string sql = "select f_id, f_station_id, f_duration, f_max_flow, UNIX_TIMESTAMP(f_update_time) AS updatetime from t_flow_control where f_valid = 1 order by f_id desc limit 10000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);
        map<string, pair<int, int>> controlTmp;
        map<string, vector<int>> flowTmp;
        map<string, size_t> positionTmp;
        map<string, int> flowCountTmp;
        unsigned int maxTime = 0;
        int maxFlowID = 0;

        for (size_t i = 0; i < data.size(); i++)
        {
            if (maxFlowID == 0)
            {
                maxFlowID = TC_Common::strto<int>(data[i]["f_id"]);
            }
            //control[TC_Common::trim(data[i]["f_station_id"])] = make_pair<int, int>(TC_Common::strto<int>(data[i]["f_duration"]), TC_Common::strto<int>(data[i]["f_max_flow"]));

            string stationId = TC_Common::trim(data[i]["f_station_id"]);
            int duration = TC_Common::strto<int>(data[i]["f_duration"]);
            int flow = TC_Common::strto<int>(data[i]["f_max_flow"]);
            if (duration <= 0 || flow <= 0)
            {
                TLOGERROR("error config, station:" << stationId << ", duration:" << duration << ", flow:" << flow << endl);
                continue;
            }
            controlTmp[stationId] = make_pair(duration, flow);
            pair<int, int> one = _control[stationId];
            if (one.first == 0)
            {
                flowTmp[stationId].resize(duration);
                positionTmp[stationId] = 0;
                flowCountTmp[stationId] = 0;
            }
            else if (one.first < duration)
            {
                flowTmp[stationId].resize(duration);
                flowTmp[stationId].insert(flowTmp[stationId].end(), _flow[stationId].begin(), _flow[stationId].end());
                positionTmp[stationId] = _position[stationId];
                flowCountTmp[stationId] = _flowCount[stationId];
            }
            else if (one.first > duration)
            {
                flowTmp[stationId].resize(duration);
                size_t index = _position[stationId] + one.first;
                for (size_t i = 0; i < (size_t)duration; i++)
                {
                    flowTmp[stationId][duration - i - 1] = _flow[stationId][index % one.first];
                    if (i > 0)
                    {
                        flowCountTmp[stationId] += flowTmp[stationId][duration - i - 1];
                    }
                }
                positionTmp[stationId] = duration - 1;
            }
            else
            {
                flowTmp[stationId] = _flow[stationId];
                positionTmp[stationId] = _position[stationId];
                flowCountTmp[stationId] = _flowCount[stationId];
            }

            unsigned int t = TC_Common::strto<unsigned int>(data[i]["updatetime"]);
            if (t > maxTime)
            {
                maxTime = t;
            }
        }

        {
            //TC_ThreadWLock w(_rwLock);
            TLOGDEBUG("control:" << TC_Common::tostr(_control) << ", new control:" << TC_Common::tostr(controlTmp) << endl);
            _control.swap(controlTmp);
            TLOGDEBUG("flow:" << TC_Common::tostr(_flow) << ", new flow:" << TC_Common::tostr(flowTmp) << endl);
            _flow.swap(flowTmp);
            TLOGDEBUG("postion:" << TC_Common::tostr(_flow) << ", new postion:" << TC_Common::tostr(flowTmp) << endl);
            _position.swap(positionTmp);
            TLOGDEBUG("flowcount:" << TC_Common::tostr(_flow) << ", new flowcount:" << TC_Common::tostr(flowTmp) << endl);
            _flowCount.swap(flowCountTmp);
            _maxFlowID = maxFlowID;
            _latestDBUpdateTime = maxTime;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }
    return false;
}

bool FlowControlManager::check(const string &stationId)
{
    TC_LockT<TC_ThreadMutex> lock(_mutex);
    if ((!_onoff) || (_flowRemain.find(stationId) == _flowRemain.end()))
    {
        // 没有打开开关 或者 没有对应配置，则不限制
        return true;
    }

    if (_flowSecond[stationId] >= _flowRemain[stationId])
    {
        return false;
    }

    _flowSecond[stationId]++;
    return true;
}

int FlowControlManager::report(const map<string, int> &flow, const string &ip)
{
    TLOGDEBUG("ip:" << ip << ", flow:" << TC_Common::tostr(flow) << endl);

    TC_LockT<TC_ThreadMutex> lock(_mutex);
    for (auto it = flow.begin(); it != flow.end(); ++it)
    {
        _flowRemain[it->first] -= it->second;
        _flowSecondReport[it->first] += it->second;
    }

    return 0;
}

void FlowControlManager::doReport(map<string, int> &flow)
{
    if (_reportObj.empty())
    {
        return;
    }

    // 过滤掉站点为空的上报内容
    for (auto it = flow.begin(); it != flow.end();)
    {
        if (it->second <= 0)
        {
            flow.erase(it++);
        }
        else
        {
            it++;
        }
    }

    if (flow.empty())
    {
        return;
    }

    time_t lastFlushEpTime = 0;
    if (_reportEpList.size() == 0 || TNOW > lastFlushEpTime + 60)
    {
        _reportEpList = Application::getCommunicator()->getEndpoint4All(_reportObj);
        lastFlushEpTime = TNOW;
    }

    for (size_t i = 0; i < _reportEpList.size(); i++)
    {
        TC_Endpoint &ep = _reportEpList[i];
        if (ep.getHost() == _localIp)
        {
            continue;
        }
        Base::FlowControlPrx prx = Application::getCommunicator()->stringToProxy<Base::FlowControlPrx>(_reportObj + "@" + ep.toString());
        prx->async_report(NULL, flow, _localIp);
    }
}

void FlowControlManager::run()
{
    int round = 0;
    while (!_terminate)
    {
        try
        {
            map<string, int> flowTmp;
            {
                TC_LockT<TC_ThreadMutex> lock(_mutex);
                map<string, int> flowRemain;
                for (auto it = _flow.begin(); it != _flow.end(); ++it)
                {
                    size_t index = _position[it->first] % (it->second.size());
                    it->second[index] = _flowSecond[it->first] + _flowSecondReport[it->first];
                    _flowCount[it->first] = _flowCount[it->first] + it->second[index] - it->second[(index + 1) % it->second.size()];
                    flowRemain[it->first] = max(_control[it->first].second - _flowCount[it->first], 0);
                    if (flowRemain[it->first] == 0)
                    {
                        TLOGDEBUG("station:" << it->first << ", index:" << index << ", flow:" << TC_Common::tostr(it->second) << endl);
                    }
                    _position[it->first] = (_position[it->first] + 1) % it->second.size();
                }
                //TLOGDEBUG("flowsecond:" << TC_Common::tostr(_flowSecond) << endl);
                flowTmp.swap(_flowSecond);
                //TLOGDEBUG("flowTmp:" << TC_Common::tostr(flowTmp) << endl);
                _flowSecond.clear();
                _flowSecondReport.clear();
                _flowRemain.swap(flowRemain);
            }

            doReport(flowTmp);

            if (round++ >= 300)
            {
                round = 0;
                loadDB();
            }
        }
        catch (exception &ex)
        {
            TLOGERROR("exception:" << ex.what() << endl);
        }
        catch (...)
        {
            TLOGERROR("exception unknown error." << endl);
        }

        TC_ThreadLock::Lock lock(*this);
        timedWait(1000);
    }
}