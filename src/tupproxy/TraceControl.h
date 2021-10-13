#ifndef _TraceControl_H_
#define _TraceControl_H_

#include <map>
#include <string>
#include "util/tc_timeprovider.h"
#include "util/tc_singleton.h"
#include "util/tc_config.h"

using namespace std;
using namespace tars;

class TraceControl: public TC_Singleton<TraceControl>
{

public:
    TraceControl();
    ~TraceControl();
    bool init(const TC_Config& conf);

    /*
    * 判断当前请求是否采用追踪调用
    */
    int check(const string& servant, const string& func);

    unsigned int getParamMaxLen() const
    {
        return _maxParamLen;
    }

protected:
    

private:
    bool        _onoff;     // 是否打开调用链
    int64_t     _defaultInterval;   // 默认采样时间间隔, 单位 ms
    int         _defaultTraceType;  // 默认trace参数打印控制
    unsigned int    _maxParamLen;   // 参数最大长度
    unordered_map<string, int64_t>    _servantFuncTime;     // servant_func -> last trace time(ms)
    vector<TC_ThreadMutex*> _locks;
};



#endif