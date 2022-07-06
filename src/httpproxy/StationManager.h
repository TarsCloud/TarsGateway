#ifndef _STATIONMANAGER_H_
#define _STATIONMANAGER_H_

#include "util/tc_singleton.h"
#include "util/tc_monitor.h"
#include "servant/Application.h"
#include "util/tc_thread_rwlock.h"
#include "proxybase/ProxyParam.h"
#include "util/tc_mysql.h"
#include "HttpRouter.h"

using namespace std;
using namespace tars;


struct ProxyAddrInfo
{
    //vector<pair<string, int>>   addrList;
    vector<UpstreamInfo> addrList;
    string ver;
};


/**
 * 管理站点配置信息
 */
class StationManager : public TC_Singleton<StationManager>
                      , public TC_ThreadLock
                      , public TC_Thread
{
public:
    StationManager();

    bool init(TC_Config& conf);
     /**
     * 从db中加载后端配置信息
     */
    bool load(TC_Config& conf);
    bool loadHttp(TC_Config& conf);
    bool loadComm();
    void addObjUpstream(const string &obj);

    string getMonitorUrl(const string& stationId);
    bool isInBlackList(const string &stationId, const string &ip);
    bool checkWhiteList(const string &stationId, const string &ip);
    /**
     * 结束
     */
    void terminate();

protected:
    /**
     * run
     */
    virtual void run();

    bool loadHttpRouterConf(vector<RouterParam>& paramList);
    bool loadUpstream();
    bool loadRouter();
    void flushObj();
    bool loadMonitor();
    bool loadBlackList();
    bool loadWhiteList();

    string genAddrVer(const vector<UpstreamInfo>& addrList);

  protected:
    bool                _terminate{false};
    TC_ThreadRWLocker   _rwLock;
    // station monitor check
    unordered_map<string, string> _stationMonitorUrl;
    // stationid, obj
    set<string> _stationObj; 
    unordered_map<string, ProxyAddrInfo> _ObjProxy; 

    unordered_map<string, set<string>> _blackList;
    unordered_map<string, set<string>> _blackListPat;

    unordered_map<string, set<string>> _whiteList;
    unordered_map<string, set<string>> _whiteListPat;

    TC_Mysql _mysql;

    bool _isLoading;
    TC_Config _conf;
};

#define STATIONMNG  StationManager::getInstance()

#endif
