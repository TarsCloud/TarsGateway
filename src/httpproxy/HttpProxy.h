#ifndef _HTTPPROXY_H_
#define _HTTPPROXY_H_

#include "util/tc_singleton.h"
#include "util/tc_monitor.h"
#include "servant/Application.h"
#include "util/tc_thread_rwlock.h"
#include "util/tc_monitor.h"
#include "proxybase/ProxyParam.h"

using namespace std;
using namespace tars;
/**
 * 移植自tarsrpc超时切换逻辑
 * 
 * 超时一定比率后进行切换
 * 设置超时检查参数
 * 计算到某台服务器的超时率, 如果连续超时次数或者超时比例超过阀值
 * 默认60s内, 超时调用次数>=2, 超时比率0.5或者连续超时次数>5,
 * 则失效
 * 服务失效后, 请求将尽可能的切换到其他可能的服务器, 并每隔tryTimeInterval尝试一次, 如果成功则认为恢复
 * 如果其他服务器都失效, 则随机选择一台尝试
 * @uint16_t minTimeoutInvoke, 计算的最小的超时次数, 默认2次(在checkTimeoutInterval时间内超过了minTimeoutInvoke, 才计算超时)
 * @uint32_t frequenceFailInvoke, 连续失败次数
 * @uint32_t checkTimeoutInterval, 统计时间间隔, (默认60s, 不能小于30s)
 * @float radio, 超时比例 > 该值则认为超时了 ( 0.1<=radio<=1.0 )
 * @uint32_t tryTimeInterval, 重试时间间隔
 */
class AddrProxy: public TC_HandleBase
{
public:
    enum E_ADDR_STATUS
    {
        EAS_SUCC = 0,             // 可用
        EAS_WEIGHT = 1,           // 由于权重，当前节点权限轮训已经完成
        EAS_TIMEOUT = 2,          // 超时暂时屏蔽
        EAS_TIMEOUT_WEIGHT = 3,   // 超时且权重轮训完
        EAS_INACTIVE = 4,         // 节点不可用屏蔽
        EAS_INACTIVE_WEIGHT = 5,  // 节点不可用屏蔽且超轮训次数
        EAS_FAIL = 99,
    };

    AddrProxy(const string &upstream, const string &addr, int weight, bool fusingOnOff);

    void update(int weight, bool fusingOnOff)
    {
        _weight = weight;
        _fusingOnOff = fusingOnOff;
    }

    ~AddrProxy()
    {
      TLOG_ERROR(getID() << " destroy." << endl);
    }
    //void finishCall(E_HTTP_RESLUT reslut);
    void doFinish(bool bFail);
    bool isActive()
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        return _isActive;
    }
    bool setActive(bool active)
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        if (_isActive == active)
        {
            return false;
        }
        _isActive = active;
        return true;
    }

    E_ADDR_STATUS available();

    void incCount()
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        if (_count >= _weight)
        {
            _count = 0;
        }
        else
        {
            _count++;
        }
    }

    void destroy()
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        _isValid = false;
    }

    string getID()
    {
        return _upstream + "@" + _addr;
    }
    // 被设置为inactive后，轮训检测是否恢复
    bool check();
    // 检测端口能否connect, timeout 超时时间 单位秒
    static bool checkConnect(const string &host, uint16_t port, int timeout);

    string getAddr() const
    {
        return _addr;
    }

    bool fusingOnOff() const
    {
        return _fusingOnOff;
    }

  protected:
    // 如果没有monitorurl 的情况下，直接connect端口是否能连通
    bool doTcpMonitor(const string& addr);
    // 根据配置的monitorurl进行检测 
    bool doHttpMonitor(const string &url);

    void setTimeout(bool isTimeout)
    {
        _isTimeout = isTimeout;
    }

  
  private:
    
    string _upstream;
    string _addr;
    int _weight;
    int _count;
    bool _fusingOnOff;

    time_t _nextCheckTime;
    unsigned int _lastInterval;

    TC_ThreadMutex _mutex;
    time_t _lastReqTime;
    bool _isValid;
    bool _isActive;
    bool _isTimeout;

    uint32_t						_timeoutInvoke;
    uint32_t						_totalInvoke;
    uint32_t						_frequenceFailInvoke;
    time_t                          _nextFinishInvokeTime;
    time_t                          _nextRetryTime;
    time_t                          _frequenceFailTime;

    const static uint16_t           minTimeoutInvoke = 2;
    const static uint32_t           checkTimeoutInterval = 60;
    const static uint32_t           frequenceFailInvoke = 5;
    const static uint32_t           minFrequenceFailTime = 5;
    //const static float radio = 0.5;
    const static uint32_t           radio_100 = 50;
    const static uint32_t           tryTimeInterval = 10;    

};
typedef TC_AutoPtr<AddrProxy> AddrPrx;


/**
 * 管理站点配置信息
 */
class HttpProxy : public TC_ThreadMutex, public TC_HandleBase
{
public:

    HttpProxy(const string& upstream);
    ~HttpProxy();
    
    AddrPrx getProxy();
    void setAddr(const vector<UpstreamInfo>& addrList, const string& ver);

private:
    
    string _upstream;
    //time_t _addrUpdateTime;
    string _addrVer;
    vector<string> _addrList;

    unordered_map<string, AddrPrx> _addrPrxList;
    unordered_map<string, AddrPrx>::iterator _prxIterator;
};

typedef TC_AutoPtr<HttpProxy> HttpPrx;

class HttpProxyFactory: public TC_ThreadRecMutex, public TC_Singleton<HttpProxyFactory>
{
public:
    HttpPrx getHttpProxy(const string& upstream);

    //static HttpPrx getHttpProxy(const string& upstream); 
protected:

    unordered_map<string, HttpPrx> _proxy;

};

class AddrCheckThread : public TC_Singleton<AddrCheckThread>
                      , public TC_ThreadLock
                      , public TC_Thread
{
public:
    AddrCheckThread() :  _terminate(false), _index(0)
    {
        start();
    }

    void addAddr(AddrPrx prx)
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        _addr[_index][prx->getID()] = prx;
    }

    /**
     * 结束
     */
    void terminate()
    {
        _terminate = true;
    }

  protected:
    /**
     * run
     */
    virtual void run();

  protected:
    bool                _terminate;

    TC_ThreadMutex      _mutex;
    unordered_map<string, AddrPrx>   _addr[2];
    size_t _index;
};

#endif