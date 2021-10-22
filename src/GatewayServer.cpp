#include "GatewayServer.h"
#include "FlowControlImp.h"
#include "proxybase/FlowControlManager.h"
#include "ProxyImp.h"
#include "httpproxy/StationManager.h"
#include "tupproxy/TupBase.h"
#include "tupproxy/TupProxyManager.h"
#include "util/tc_http.h"
#include "util/tc_network_buffer.h"
//include "gperftools/profiler.h"
#include "tupproxy/TraceControl.h"

using namespace std;

GatewayServer g_app;

//string tupProxyConf;

/////////////////////////////////////////////////////////////////
bool GatewayServer::loadProxy(const string &command, const string &params, string &result)
{
    TLOG_DEBUG("command:" << command << endl);
    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    result = TupProxyManager::getInstance()->loadProxy(conf);
    TLOG_DEBUG("result:" << result << endl);
    return true;
}

bool GatewayServer::loadHttp(const string &command, const string &params, string &result)
{
    TLOG_DEBUG("[GatewayServer::loadHttp] command:" << command << endl);
    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    result = STATIONMNG->loadHttp(conf) ? "succ" : "fail";
    TLOG_DEBUG("[GatewayServer::load] result:" << result << endl);
    return true;
}

bool GatewayServer::loadComm(const string &command, const string &params, string &result)
{
    TLOG_DEBUG("[GatewayServer::loadHttp] command:" << command << endl);
    result = STATIONMNG->loadComm() ? "succ" : "fail";
    TLOG_DEBUG("[GatewayServer::load] result:" << result << endl);
    return true;
}

bool GatewayServer::loadRspHeader(const string &command, const string &params, string &result)
{
    try
    {
        addConfig("httpheader.conf");
        TC_Config conf;
        conf.parseFile(ServerConfig::BasePath + "httpheader.conf");
        map<string, string> protoMap = conf.getDomainMap("/httprsp_headers/protocol_map");

        map<string, map<string, string>> headers;
        for (auto it = protoMap.begin(); it != protoMap.end(); ++it)
        {
            map<string, string> defHeaders = conf.getDomainMap("/httprsp_headers/" + it->second + "/default_headers");
            if (defHeaders.size() > 0)
            {
                headers[it->first] = defHeaders;
                TLOG_DEBUG("add http header:" << it->first << " |" << TC_Common::tostr(defHeaders) << endl);
            }

            vector<string> bidList = conf.getDomainVector("/httprsp_headers/" + it->second + "/special_headers");
            for (size_t i = 0; i < bidList.size(); i++)
            {
                string strList = conf.get("/httprsp_headers/" + it->second + "/special_headers/" + bidList[i] + "<servant_station_list>");
                vector<string> ssList = TC_Common::sepstr<string>(strList, "|");
                if (ssList.size() == 0)
                {
                    continue;
                }

                map<string, string> mh = conf.getDomainMap("/httprsp_headers/" + it->second + "/special_headers/" + bidList[i] + "/headers");
                if (mh.size() == 0)
                {
                    continue;
                }

                for (size_t i = 0; i < ssList.size(); i++)
                {
                    headers[it->first + "#" + ssList[i]] = mh;
                    TLOG_DEBUG("add http header:" << it->first + "#" + ssList[i] << " |" << TC_Common::tostr(defHeaders) << endl);
                }
            }
        }
        
        {
            TC_ThreadWLock w(_rwLock);
            _httpHeaders.swap(headers);
            TLOG_DEBUG("add rsp http headers finish, total:" << _httpHeaders.size() << endl);
        }

        result = "load succ, total:" + TC_Common::tostr(headers.size());
        return true;
    }
    catch(const std::exception& e)
    {
        TLOG_ERROR("exception:" << e.what() << endl); 
        result = "exception:" + string(e.what());
    }
    return false;
}

/////////////////////////////////////////////////////////////////
void GatewayServer::initialize()
{
    addServant<ProxyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".ProxyObj");
    //addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".TupProxyObj", &TupProtocol::parseHttp);
    addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".ProxyObj", &tars::TC_NetWorkBuffer::parseHttp);

    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    if (_tupProxyConf.empty())
    {
        _tupProxyConf = ServerConfig::BasePath + ServerConfig::ServerName + ".conf";
        //conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    }

    conf.parseFile(_tupProxyConf);

    //默认是5M
    _rspSizeLimit = TC_Common::strto<unsigned int>(conf.get("/main/base<rspsize>", "5242880"));
    _localServerName = ServerConfig::Application + "." + ServerConfig::ServerName;
    string tupHost = TC_Common::trim(conf.get("/main/base<tup_host>", ""));
    vector<string> whList = TC_Common::sepstr<string>(tupHost, "|");
    for (size_t i = 0; i < whList.size(); i++)
    {
        if (whList[i].size() > 0 && whList[i][0] == '*')
        {
            _tupPreHost.push_back(TC_Common::trim(whList[i].substr(1)));
        }
        else
        {
            _tupFullHost.insert(TC_Common::trim(whList[i]));
        }
    }

    _tupPath = TC_Common::trim(conf.get("/main/base<tup_path>", "/tup"));
    if (_tupPath.back() == '/')
    {
        _tupPath.pop_back();
    }
    _jsonPath = TC_Common::trim(conf.get("/main/base<json_path>", "/json"));
    if (_jsonPath.back() == '/')
    {
        _jsonPath.pop_back();
    }
    _jsonPathEx = _jsonPath + "/";
    _monitorUrl = TC_Common::trim(conf.get("/main/base<monitor_url>", "/monitor/monitor.html"));

    TLOG_DEBUG("_rspSizeLimit:" << _rspSizeLimit
                               << ", _localServerName:" << _localServerName
                               << ", _tupHost:" << tupHost
                               << ", tupfullhost:" << TC_Common::tostr(_tupFullHost.begin(), _tupFullHost.end())
                               << ", tupprehost:" << TC_Common::tostr(_tupPreHost.begin(), _tupPreHost.end())
                               << ", _tupPath:" << _tupPath
                               << ", _jsonPath:" << _jsonPath
                               << ", _monitorUrl:" << _monitorUrl << endl);

    string httpRetCode = conf.get("/main/http_retcode<inactive>", "");
    vector<int> vi = TC_Common::sepstr<int>(httpRetCode, "|");
    for (size_t i = 0; i < vi.size(); i++)
    {
        _inactiveRetCode.insert(vi[i]);
    }
    httpRetCode = conf.get("/main/http_retcode<timeout>", "");
    vi = TC_Common::sepstr<int>(httpRetCode, "|");
    for (size_t i = 0; i < vi.size(); i++)
    {
        _timeoutRetCode.insert(vi[i]);
    }
    TLOG_DEBUG("inactive code:" << TC_Common::tostr(_inactiveRetCode.begin(), _inactiveRetCode.end()) << ", timeout code:" << TC_Common::tostr(_timeoutRetCode.begin(), _timeoutRetCode.end()) << endl);

    TupBase::initStaticParam(conf);
    TupProxyManager::getInstance()->loadProxy(conf);
    //TupProxyManager::getInstance()->start();

    TraceControl::getInstance()->init(conf);

    StationManager::getInstance()->init(conf);
    StationManager::getInstance()->start();

    string loadRsp;
    loadRspHeader("", "", loadRsp);
    TLOG_DEBUG("load http header, " << loadRsp);

    TARS_ADD_ADMIN_CMD_NORMAL("loadProxy", GatewayServer::loadProxy);
    TARS_ADD_ADMIN_CMD_NORMAL("loadHttp", GatewayServer::loadHttp);
    TARS_ADD_ADMIN_CMD_NORMAL("loadComm", GatewayServer::loadComm);
    TARS_ADD_ADMIN_CMD_NORMAL("loadRspHeader", GatewayServer::loadRspHeader);

    try
    {
        addServant<FlowControlImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".FlowControlObj");
        FlowControlManager::getInstance()->init(conf);
        FlowControlManager::getInstance()->start();
        TLOG_DEBUG("add flowcontrol obj succ..." << endl);
    }
    catch (const std::exception &e)
    {
        TLOG_ERROR("add flowcontrol fail, exception:" << e.what() << endl);
    }
}

bool GatewayServer::isTupHost(const string &h) const
{
    if (_tupFullHost.size() == 0 && _tupPreHost.size() == 0)
    {
        return true;
    }

    if (_tupFullHost.find(h) != _tupFullHost.end())
    {
        return true;
    }

    for (size_t i = 0; i < _tupPreHost.size(); i++)
    {
        if (h.length() > _tupPreHost[i].length() && strncmp(h.c_str() + (h.length() - _tupPreHost[i].length()), _tupPreHost[i].c_str(), _tupPreHost[i].length()) == 0)
        {
            return true;
        }
    }
    return false;
}

void GatewayServer::setRspHeaders(int proto, const string& ss, TC_HttpResponse& httpResponse)
{
    vector<string> hkey;
    hkey.push_back(TC_Common::tostr(proto));
    hkey.push_back(TC_Common::tostr(proto) + "#" + ss);

    TC_ThreadRLock r(_rwLock);

    for (auto& k : hkey)
    {
        auto it = _httpHeaders.find(k);
        if (it != _httpHeaders.end())
        {
            for (auto ith = it->second.begin(); ith != it->second.end(); ++ith)
            {
                httpResponse.setHeader(ith->first, ith->second);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////
void GatewayServer::destroyApp()
{
    // if(TupProxyManager::getInstance()->isAlive())
    // {
    //     TupProxyManager::getInstance()->terminate();

    //     TupProxyManager::getInstance()->getThreadControl().join();
    // }
}

/////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    //ProfilerStart("GatewayServer.prof");

    try
    {
        //        TC_Common::ignorePipe();
        TC_Option option;
        option.decode(argc, argv);
        //tupProxyConf = option.getValue("tup");
        g_app.setConfFile(option.getValue("tup"));
        g_app.main(argc, argv);
        g_app.waitForShutdown();

        //		ProfilerStop();
    }
    catch (std::exception &e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
