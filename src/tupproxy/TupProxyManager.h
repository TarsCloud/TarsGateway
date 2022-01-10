#ifndef _TUPPROXYMANAGER_H_
#define _TUPPROXYMANAGER_H_

#include "servant/Application.h"
#include "util/tc_monitor.h"
#include "util/tc_singleton.h"
#include "proxybase/ProxyParam.h"

using namespace std;
using namespace tars;

/**
 * 管理所有后端的TARS服务的代理
 */
class TupProxyManager : public TC_Singleton<TupProxyManager>, public TC_ThreadLock
// , public TC_Thread
{
  public:
    //friend typename TC_Singleton<TupProxyManager>::TCreatePolicy;

    /**
     * 加载配置
     * 
     * @return string 
     */
    string loadProxy(const TC_Config &conf);

    /**
     * 获取代理
     * 
     * @param tup 
     * 
     * @return ServantPrx 
     */
    ServantPrx getProxy(const string &sServantName, const string &sFuncName, const TC_HttpRequest &httpRequest, THashInfo &hi);
    ServantPrx getProxy(const string& sServantName, const string& sFuncName, const TC_HttpRequest &httpRequest, ProxyExInfo& pei);
    /**
     * 构造
     */
    TupProxyManager();

    /**
     * 结束
     */
    //void terminate();

  protected:
    //virtual void run();

    string parseHashInfo(const string &objInfo, THashInfo &hi);
    void updateHashInfo(const string &servantName, const string &obj);
    void initVerifyInfo(const TC_Config& conf);

  protected:
    TC_ThreadMutex _mutex;

    ProxyProtocol _prot_tup; //tup

    map<string, string> _nameMap;

    map<string, pair<ServantPrx, THashInfo>> _proxyMap;
    map<string, VerifyInfo>     _proxyVerify;
    set<string>                 _noVerify;

    set<string> _realnameSet;

    set<string> _httpHeaderForProxy;

    map<string, string> _httpHeader;

    time_t _lastUpdateTime;
    int _lastUpdateTotalNum;

    bool _autoProxy;
};

/////////////////////////////////////////////////////
#endif
