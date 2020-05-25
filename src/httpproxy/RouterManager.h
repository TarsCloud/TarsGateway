#ifndef _ROUTERMANAGER_H_
#define _ROUTERMANAGER_H_

#include "util/tc_singleton.h"
#include "util/tc_monitor.h"
#include "servant/Application.h"
#include "util/tc_thread_rwlock.h"

using namespace std;
using namespace tars;

/**
 * 管理站点配置信息
 */
class RouterManager : public TC_Singleton<RouterManager>
{
public:
     RouterManager();

    bool getStation(const string& url, string& station, string& funcPath);

    void updateRouter(const unordered_map<string, string> &router, const string& ver);

  protected:
    //void updateRouter();

protected:

    TC_ThreadRWLocker   _rwLock;

    unordered_map<string, string> _router;
    map<size_t, set<string>> _routerDepth;
    size_t _maxDepth;
    string _ver;
    //unsigned int _lastUpdateTime;
};

#define ROUTERMNG RouterManager::getInstance()

#endif