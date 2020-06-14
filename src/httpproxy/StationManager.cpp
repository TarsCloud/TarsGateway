#include "StationManager.h"
#include "HttpProxy.h"
#include "util/tc_md5.h"
#include "util/tc_mysql.h"

StationManager::StationManager()
{
    _isLoading = false;
}

bool StationManager::load(TC_Config &conf)
{
    _conf = conf;
    // 这里用个状态位进行简单处理，不是很严谨
    if (_isLoading)
    {
        return false;
    }

    _isLoading = true;
    bool ret = loadUpstream();
    ret &= loadRouter();
    ret &= loadMonitor();
    ret &= loadBlackList();
    ret &= loadWhiteList();
    _isLoading = false;

    if (ret)
    {
        flushObj();
    }

    return ret;
}

bool StationManager::loadHttp(TC_Config &conf)
{
    _conf = conf;
    // 这里用个状态位进行简单处理，不是很严谨
    if (_isLoading)
    {
        return false;
    }

    _isLoading = true;
    bool ret = loadUpstream();
    ret &= loadRouter();
    ret &= loadMonitor();
    _isLoading = false;

    if (ret)
    {
        flushObj();
    }
    return ret;
}

bool StationManager::loadComm()
{
    bool ret = loadBlackList();
    ret &= loadWhiteList();
    return ret;
}

bool StationManager::init(TC_Config &conf)
{
    map<string, string> m = conf.getDomainMap("/main/db");
    TC_DBConf tcDBConf;
    tcDBConf.loadFromMap(m);
    _mysql.init(tcDBConf);

    return load(conf);
}

bool StationManager::loadHttpRouterConf(vector<RouterParam> &paramList)
{
    try
    {
        vector<string> stationList = _conf.getDomainVector("/main/http_router");
        TLOGDEBUG("stationList:" << stationList.size() << endl);
        for (size_t i = 0; i < stationList.size(); i++)
        {
            RouterParam rp;
            rp.stationId = stationList[i];
            map<string, string> m = _conf.getDomainMap("/main/http_router/" + rp.stationId);
            rp.serverName = m["server_name"];
            rp.location = m["location"];
            rp.proxyPass = m["proxy_pass"];
            if (rp.location.empty() || rp.proxyPass.empty())
            {
                TLOGERROR("error conf:" << rp.stationId << ", " << TC_Common::tostr(m) << endl);
                continue;
            }
            paramList.push_back(rp);
        }

        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR(e.what() << endl);
    }

    return false;
}

bool StationManager::loadRouter()
{
    vector<RouterParam> paramList;
    try
    {
        string sql = "select f_id, f_station_id, f_server_name, f_path_rule, f_proxy_pass, UNIX_TIMESTAMP(f_update_time) AS updatetime from t_http_router where f_valid = 1 and f_id >= 1000 order by f_id limit 100000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);
        for (size_t i = 0; i < data.size(); i++)
        {
            RouterParam rp;
            rp.id = TC_Common::strto<int>(data[i]["f_id"]);
            rp.serverName = TC_Common::trim(data[i]["f_server_name"]);
            rp.location = TC_Common::trim(data[i]["f_path_rule"]);
            rp.proxyPass = TC_Common::trim(data[i]["f_proxy_pass"]);
            rp.stationId = TC_Common::trim(data[i]["f_station_id"]);
            if (rp.location.empty() || rp.proxyPass.empty())
            {
                TLOGERROR("error db conf:" << rp.id << "|" << rp.serverName << "|" << rp.location << "|" << rp.proxyPass << "|" << rp.stationId << endl);
                continue;
            }

            paramList.push_back(rp);
        }
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }

    // 合并配置文件内容
    loadHttpRouterConf(paramList);

    return Router->reload(paramList);
    ;
}

void StationManager::terminate()
{
}

bool StationManager::loadMonitor()
{
    try
    {
        string sql = "select f_station_id, f_monitor_url from t_station where f_valid = 1 limit 10000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);
        unordered_map<string, string> monitorUrl;
        for (size_t i = 0; i < data.size(); i++)
        {
            monitorUrl[TC_Common::trim(data[i]["f_station_id"])] = TC_Common::trim(data[i]["f_monitor_url"]);
        }

        {
            TC_ThreadWLock w(_rwLock);
            _stationMonitorUrl.swap(monitorUrl);
        }
        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }

    return false;
}

bool StationManager::loadBlackList()
{
    try
    {
        string sql = "select f_station_id, f_ip from t_blacklist where f_valid = 1 limit 10000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);

        unordered_map<string, set<string>> blackList;
        unordered_map<string, set<string>> blackListPat;
        for (size_t i = 0; i < data.size(); i++)
        {
            string stationId = TC_Common::trim(data[i]["f_station_id"]);
            string ip = TC_Common::trim(data[i]["f_ip"]);
            if (ip.find("*") != string::npos)
            {
                blackListPat[stationId].insert(ip);
            }
            else
            {
                blackList[stationId].insert(ip);
            }
        }

        {
            TC_ThreadWLock w(_rwLock);
            _blackList.swap(blackList);
            _blackListPat.swap(blackListPat);
        }
        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }

    return false;
}
bool StationManager::loadWhiteList()
{
    try
    {
        string sql = "select f_station_id, f_ip from t_whitelist where f_valid = 1 limit 100000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);
        unordered_map<string, set<string>> whiteList;
        unordered_map<string, set<string>> whiteListPat;
        for (size_t i = 0; i < data.size(); i++)
        {
            string stationId = TC_Common::trim(data[i]["f_station_id"]);
            string ip = TC_Common::trim(data[i]["f_ip"]);
            if (ip.find("*") != string::npos)
            {
                whiteListPat[stationId].insert(ip);
            }
            else
            {
                whiteList[stationId].insert(ip);
            }
        }

        {
            TC_ThreadWLock w(_rwLock);
            _whiteList.swap(whiteList);
            _whiteListPat.swap(whiteListPat);
        }
        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }

    return false;
}

void StationManager::run()
{
    while (!_terminate)
    {
        try
        {
            flushObj();
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
        timedWait(1000 * 30);
    }
}

string StationManager::getMonitorUrl(const string &stationId)
{
    TC_ThreadRLock r(_rwLock);
    auto it = _stationMonitorUrl.find(stationId);
    if (it != _stationMonitorUrl.end())
    {
        return it->second;
    }

    return "";
}

// string StationManager::genAddrVer(vector<string>& addrList)
// {
//     std::sort(addrList.begin(), addrList.end());
//     stringstream ss;
//     for (auto itv = addrList.begin(); itv != addrList.end(); ++itv)
//     {
//         ss << *itv;
//     }

//     return TC_MD5::md5str(ss.str());
// }

string StationManager::genAddrVer(const vector<UpstreamInfo> &addrList)
{
    stringstream ss;
    for (auto itv = addrList.begin(); itv != addrList.end(); ++itv)
    {
        ss << (*itv).addr << "#" << (*itv).weight << "#" << (*itv).fusingOnOff << "|";
    }

    return TC_MD5::md5str(ss.str());
}

void StationManager::addObjUpstream(const string &obj)
{
    TC_ThreadWLock w(_rwLock);
    _stationObj.insert(obj);
}

void StationManager::flushObj()
{
    set<string> obj;
    {
        TC_ThreadRLock r(_rwLock);
        obj = _stationObj;
    }

    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        vector<TC_Endpoint> ep = Application::getCommunicator()->getEndpoint4All(*it);
        if (ep.size() > 0)
        {
            vector<string> epList;
            for (size_t i = 0; i < ep.size(); i++)
            {
                epList.push_back(ep[i].getHost() + ":" + TC_Common::tostr(ep[i].getPort()));
            }

            std::sort(epList.begin(), epList.end());
            vector<UpstreamInfo> addrList;
            for (size_t i = 0; i < epList.size(); i++)
            {
                // obj 类型， 都赋默认权值1
                UpstreamInfo info;
                info.addr = epList[i];
                info.weight = 1;
                info.fusingOnOff = true;
                addrList.push_back(info);
            }

            string ver = genAddrVer(addrList);

            {
                TC_ThreadRLock r(_rwLock);
                if (ver == _ObjProxy[*it].ver)
                {
                    continue;
                }
            }

            TLOGDEBUG(*it << ", addList:" << TC_Common::tostr(epList) << endl);
            {
                TC_ThreadWLock w(_rwLock);
                _ObjProxy[*it].addrList = addrList;
                _ObjProxy[*it].ver = ver;
            }

            HttpProxyFactory::getInstance()->getHttpProxy(*it)->setAddr(addrList, ver);
        }
        else
        {
            TLOGERROR(*it << ", has no valid tars enpoint." << endl);
        }
    }
}

bool StationManager::isInBlackList(const string &stationId, const string &ip)
{
    TC_ThreadRLock r(_rwLock);
    auto it1 = _blackList.find(stationId);
    if (it1 != _blackList.end())
    {
        if (it1->second.find(ip) != it1->second.end())
        {
            return true;
        }
    }

    auto it2 = _blackListPat.find(stationId);
    if (it2 != _blackListPat.end())
    {
        for (auto it = it2->second.begin(); it != it2->second.end(); ++it)
        {
            if (TC_Common::matchPeriod(ip, *it))
            {
                return true;
            }
        }
    }

    return false;
}

bool StationManager::checkWhiteList(const string &stationId, const string &ip)
{
    // 一旦找到站点，那么当前站点就是白名单策略， ip不在白名单， 那么就不能访问。
    bool ret = true;
    TC_ThreadRLock r(_rwLock);
    auto it1 = _whiteList.find(stationId);
    if (it1 != _whiteList.end())
    {
        // 找到了该站点， ret 置为false
        ret = false;
        if (it1->second.find(ip) != it1->second.end())
        {
            return true;
        }
    }

    auto it2 = _whiteListPat.find(stationId);
    if (it2 != _whiteListPat.end())
    {
        ret = false;
        for (auto it = it2->second.begin(); it != it2->second.end(); ++it)
        {
            if (TC_Common::matchPeriod(ip, *it))
            {
                return true;
            }
        }
    }

    return ret;
}

bool StationManager::loadUpstream()
{
    try
    {
        string sql = "select f_id, f_upstream, f_addr, f_weight, f_fusing_onoff, UNIX_TIMESTAMP(f_update_time) AS updatetime from t_upstream where f_valid = 1 order by f_id desc limit 100000";
        TC_Mysql::MysqlData data = _mysql.queryRecord(sql);
        TLOGDEBUG(sql << " ===> result size=" << data.size() << endl);
        unordered_map<string, ProxyAddrInfo> proxy;

        for (size_t i = 0; i < data.size(); i++)
        {
            UpstreamInfo info;
            info.addr = TC_Common::trim(data[i]["f_addr"]);
            info.weight = TC_Common::strto<int>(data[i]["f_weight"]);
            info.fusingOnOff = (TC_Common::strto<int>(data[i]["f_fusing_onoff"]) == 1 ? true : false);
            proxy[TC_Common::trim(data[i]["f_upstream"])].addrList.push_back(info);
            //proxy[TC_Common::trim(data[i]["f_upstream"])].addrList.push_back(make_pair(TC_Common::trim(data[i]["f_addr"]), TC_Common::strto<int>(data[i]["f_weight"])));
        }

        for (auto it = proxy.begin(); it != proxy.end(); ++it)
        {
            it->second.ver = genAddrVer(it->second.addrList);
            HttpProxyFactory::getInstance()->getHttpProxy(it->first)->setAddr(it->second.addrList, it->second.ver);
        }

        return true;
    }
    catch (const std::exception &e)
    {
        TLOGERROR("exception:" << e.what() << endl);
    }

    return false;
}
