#ifndef _WUPPROXYMANAGER_H_
#define _WUPPROXYMANAGER_H_

#include "util/tc_singleton.h"
#include "util/tc_monitor.h"
#include "servant/Application.h"

using namespace std;
using namespace tars;

struct THashInfo
{
    enum E_HASH_TYPE
    {
        EHT_ROBINROUND = 0,
        EHT_REQUESTID = 1,
        EHT_HTTPHEAD = 2,
        EHT_CLIENTIP = 3,
        EHT_DEFAULT = 99,
    };

    THashInfo()
    {
        type = EHT_DEFAULT;
    }

    E_HASH_TYPE type;
    string httpHeadKey;
};

/**
 * 管理所有后端的TARS服务的代理
 */
class WupProxyManager : public TC_Singleton<WupProxyManager>
                      , public TC_ThreadLock
                      // , public TC_Thread
{
public:
    friend typename TC_Singleton<WupProxyManager>::TCreatePolicy;


    /**
     * 加载配置
     * 
     * @return string 
     */
    string loadProxy(const TC_Config& conf);

    /**
     * 获取代理
     * 
     * @param wup 
     * 
     * @return ServantPrx 
     */
    ServantPrx getProxy(const string& sServantName, const string& sFuncName, const TC_HttpRequest &httpRequest, THashInfo& hi);

    /**
     * 结束
     */
    //void terminate();

protected:
    //virtual void run();
    
    /**
     * 构造
     */
    WupProxyManager();

    string parseHashInfo(const string& objInfo, THashInfo& hi);
    void updateHashInfo(const string &servantName, const string &obj);

  protected:
    TC_ThreadMutex          _mutex;

    ProxyProtocol           _prot_wup;  //wup

    map<string, string>     _nameMap;

    //map<string, ServantPrx> _proxyMap;
    map<string, pair<ServantPrx, THashInfo>> _proxyMap;
    //map<string, ServantPrx> _jsonProxy;

    set<string>             _realnameSet;

    set<string>             _httpHeaderForProxy;

    map<string, string>     _httpHeader;

    time_t                  _lastUpdateTime;
    int                     _lastUpdateTotalNum;

    //bool                    _terminate;

    bool                    _autoProxy;
    

    //string sGUID;

};

/////////////////////////////////////////////////////
#endif
