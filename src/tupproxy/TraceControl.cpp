#include "TraceControl.h"
#include "servant/RemoteLogger.h"
#include "util/tc_hash_fun.h"

#define CONTROL_LOCK(x) TC_LockT<TC_ThreadMutex> Lock(*_locks[tars::hash<string>()(x)%_locks.size()]);

TraceControl::TraceControl(): _onoff(false), _defaultInterval(100), _defaultTraceType(0), _maxParamLen(1)
{
    _locks.clear();

    for (size_t i = 0; i < 10; i++)
    {
        _locks.push_back(new TC_ThreadMutex());
    }
}

TraceControl::~TraceControl()
{
    for (size_t i = 0; i < _locks.size(); i++)
    {
        delete _locks[i];
    }

    _locks.clear();
}

bool TraceControl::init(const TC_Config& conf)
{
    _onoff = (bool)TC_Common::strto<int>(conf.get("/main/trace<onoff>", "0"));
    _defaultInterval = TC_Common::strto<int64_t>(conf.get("/main/trace<default_interval>", "100"));
    _defaultTraceType = TC_Common::strto<int>(conf.get("/main/trace<default_trace_type>", "0"));
    _maxParamLen = TC_Common::strto<unsigned int>(conf.get("/main/trace<param_max_len>", "1"));
    
    TLOG_DEBUG("onoff:" << _onoff << ", defaultInterval:" << _defaultInterval << ", " << _defaultTraceType << ", param_max_len:" << _maxParamLen << endl);

    return true;
}

int TraceControl::check(const string& servant, const string& func)
{
    if (!_onoff)
    {
        return -1;
    }
    
    int64_t nms = TNOWMS;
    string k = servant + ":" + func;

    CONTROL_LOCK(k);
    if (_servantFuncTime[k] + _defaultInterval < nms)
    {
        _servantFuncTime[k] = nms;
        return _defaultTraceType;
    }

    return -2;
}
