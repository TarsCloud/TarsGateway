#include "GetwayServer.h"
#include "ProxyImp.h"
#include "wupproxy/WupProxyManager.h"
#include "wupproxy/WupBase.h"
#include "util/tc_http.h"
#include "util/tc_network_buffer.h"
#include "httpproxy/StationManager.h"
#include "FlowControlManager.h"
#include "FlowControlImp.h"
//include "gperftools/profiler.h"

using namespace std;

GetwayServer g_app;

//string wupProxyConf;

/////////////////////////////////////////////////////////////////
bool  GetwayServer::loadProxy(const string& command, const string& params, string& result)
{
    TLOGDEBUG("command:" << command << endl);
    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    result = WupProxyManager::getInstance()->loadProxy(conf);
    TLOGDEBUG("result:" << result << endl);
    return true;
}

bool  GetwayServer::loadHttp(const string& command, const string& params, string& result)
{
    TLOGDEBUG("[GetwayServer::loadHttp] command:" << command << endl);
    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    result = STATIONMNG->loadHttp(conf) ? "succ": "fail";
    TLOGDEBUG("[GetwayServer::load] result:" << result << endl);
    return true;
}

bool  GetwayServer::loadComm(const string& command, const string& params, string& result)
{
    TLOGDEBUG("[GetwayServer::loadHttp] command:" << command << endl);
    result = STATIONMNG->loadComm() ? "succ": "fail";
    TLOGDEBUG("[GetwayServer::load] result:" << result << endl);
    return true;
}

/////////////////////////////////////////////////////////////////
void
GetwayServer::initialize()
{
    addServant<ProxyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".WupProxyObj");
    //addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".WupProxyObj", &WupProtocol::parseHttp);
	addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".WupProxyObj", &tars::TC_NetWorkBuffer::parseHttp);

    addConfig(ServerConfig::ServerName + ".conf");
    TC_Config conf;
    if(_wupProxyConf.empty()) {
        _wupProxyConf = ServerConfig::BasePath + ServerConfig::ServerName + ".conf";
        //conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    }

	conf.parseFile(_wupProxyConf);


    //默认是5M
    _rspSizeLimit = TC_Common::strto<unsigned int>(conf.get("/main/base<rspsize>", "5242880"));
    _localServerName = ServerConfig::Application + "." + ServerConfig::ServerName;
    string wupHost = TC_Common::trim(conf.get("/main/base<wup_host>", ""));
    vector<string> whList = TC_Common::sepstr<string>(wupHost, "|");
    for (size_t i = 0; i < whList.size(); i++)
    {
        if (whList[i].size() > 0 && whList[i][0] == '*')
        {
            _wupPreHost.push_back(TC_Common::trim(whList[i].substr(1)));
        }
        else
        {
            _wupFullHost.insert(TC_Common::trim(whList[i]));
        }  
    }
    
    _wupPath = TC_Common::trim(conf.get("/main/base<wup_path>", "/wup"));
    if (_wupPath.back() == '/')
    {
        _wupPath.pop_back();
    }
    _jsonPath = TC_Common::trim(conf.get("/main/base<json_path>", "/json"));
    if (_jsonPath.back() == '/')
    {
        _jsonPath.pop_back();
    }
    _jsonPathEx = _jsonPath + "/";
    _monitorUrl = TC_Common::trim(conf.get("/main/base<monitor_url>", "/monitor/monitor.jsp"));

    TLOGDEBUG("_rspSizeLimit:" << _rspSizeLimit
                                << ", _localServerName:" << _localServerName
                                << ", _wupHost:" << wupHost
                                << ", _wupPath:" << _wupPath
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
    TLOGDEBUG("inactive code:" << TC_Common::tostr(_inactiveRetCode.begin(), _inactiveRetCode.end()) << ", timeout code:" << TC_Common::tostr(_timeoutRetCode.begin(), _timeoutRetCode.end()) <<endl);

    WupBase::initStaticParam(conf);
    WupProxyManager::getInstance()->loadProxy(conf);
    //WupProxyManager::getInstance()->start();

    StationManager::getInstance()->init(conf);
    StationManager::getInstance()->start();

    TARS_ADD_ADMIN_CMD_NORMAL("loadProxy", GetwayServer::loadProxy);
    TARS_ADD_ADMIN_CMD_NORMAL("loadHttp", GetwayServer::loadHttp);
    TARS_ADD_ADMIN_CMD_NORMAL("loadComm", GetwayServer::loadComm);

    try
    {
        addServant<FlowControlImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".FlowControlObj");
        FlowControlManager::getInstance()->init(conf);
        FlowControlManager::getInstance()->start();
        TLOGDEBUG("add flowcontrol obj succ..." << endl);
    }
    catch(const std::exception& e)
    {
        TLOGERROR("add flowcontrol fail, exception:" << e.what() << endl);
    }    
}

bool GetwayServer::isWupHost(const string &h) const
{
    if (_wupFullHost.size() == 0 && _wupPreHost.size() == 0)
    {
        return true;
    }
    
    if (_wupFullHost.find(h) != _wupFullHost.end())
    {
        return true;
    }
    
    for (size_t i = 0; i < _wupPreHost.size(); i++)
    {
        if (h.length() > _wupPreHost[i].length() && strncmp(h.c_str() + (h.length() - _wupPreHost[i].length()), _wupPreHost[i].c_str(), _wupPreHost[i].length()) == 0)
        {
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////
void GetwayServer::destroyApp()
{
    // if(WupProxyManager::getInstance()->isAlive())
    // {
    //     WupProxyManager::getInstance()->terminate();

    //     WupProxyManager::getInstance()->getThreadControl().join();
    // }
}

/////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//ProfilerStart("GetwayServer.prof");

	try
    {
//        TC_Common::ignorePipe();
	    TC_Option option;
	    option.decode(argc, argv);
	    //wupProxyConf = option.getValue("wup");
        g_app.setConfFile(option.getValue("wup"));
        g_app.main(argc, argv);
        g_app.waitForShutdown();

//		ProfilerStop();

	}
    catch (std::exception& e)
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
