#ifndef _FLOWCONTROLMANAGER_H_
#define _FLOWCONTROLMANAGER_H_

#include "util/tc_singleton.h"
#include "util/tc_monitor.h"
#include "servant/Application.h"
#include "util/tc_thread_rwlock.h"
#include "util/tc_mysql.h"

using namespace std;
using namespace tars;


/**
 * 管理站点配置信息
 */
class FlowControlManager : public TC_Singleton<FlowControlManager>
                      , public TC_ThreadLock
                      , public TC_Thread
{
public:
    FlowControlManager();

    bool init(TC_Config& conf);
    /**
     * 从db中加载后端配置信息
     */
    bool loadDB();

    bool check(const string& stationId);
    int report(const map<string, int>& flow, const string& ip);

    int getDBConf(map<string, string>& dbConf) const
    {
        dbConf = _dbConf;
        return 0;
    }

    /**
     * 结束
     */
    void terminate();

protected:
    /**
     * run
     */
    virtual void run();

    void doReport(map<string, int>& flow);

  protected:
    bool                _terminate;
    /////////////////////////////////////////////////
    TC_ThreadMutex      _mutex;
    // 一秒请求量流量计数
    map<string, int>    _flowSecond;
    // 滑动窗口剩下的流量计数
    map<string, int>    _flowRemain;
    map<string, int> _flowSecondReport;
    /////////////////////////////////////////////////

    // 限流时间窗口每秒的详细流量
    map<string, vector<int>> _flow;
    map<string, size_t> _position;
    map<string, int> _flowCount;  // duration-1 内的流量, 本可以不用单独count计数，可以直接根据flowRemain计算得出，但是考虑到流控限制的配置可能变化， 这里就不好算， 所以还是单独记录。
    //unsigned int _index;

    map<string, string> _dbConf;

    // 限流配置信息
    map<string, pair<int, int>> _control;
    TC_Mysql _mysql;
    unsigned int _latestDBUpdateTime;
    int _maxFlowID;
    string _reportObj;
    vector<TC_Endpoint> _reportEpList;
    string _localIp;
    bool _onoff;
    
};

#endif