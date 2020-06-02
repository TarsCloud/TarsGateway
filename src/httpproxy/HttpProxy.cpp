#include "HttpProxy.h"
#include "StationManager.h"

AddrProxy::AddrProxy(const string& upstream, const string& addr, int weight, bool fusingOnOff): _isValid(true), _isActive(true), _isTimeout(false)
{
    _upstream = upstream;
    _addr = addr;
    _weight = weight;
    _count = 0;
    _fusingOnOff = fusingOnOff;
    _lastReqTime = 0;
    _lastInterval = 0;
    _nextCheckTime = 0;
    
    _timeoutInvoke = 0;
    _totalInvoke = 0;
    _frequenceFailInvoke = 0;
    _nextFinishInvokeTime = 0;
    _nextRetryTime = 0;
    _frequenceFailTime = 0;
}

AddrProxy::E_ADDR_STATUS AddrProxy::available()
{
    time_t t = TNOW;
    TC_LockT<TC_ThreadMutex> lock(_mutex);
    if (_isActive && ((!_isTimeout) || t > _lastReqTime + tryTimeInterval))
    {
        _lastReqTime = t;
        if (_count < _weight)
        {
            _count++;
            return EAS_SUCC;
        }
        else 
        {
            _count = 0;
            return EAS_WEIGHT;
        }
    }
    else if (_isActive)
    {
        if (_count < _weight)
        {
            return EAS_TIMEOUT;
        }
        else
        {
            return EAS_TIMEOUT_WEIGHT;
        }
    }
    else
    {
        if (_count < _weight)
        {
            return EAS_INACTIVE;
        }
        else
        {
            return EAS_INACTIVE_WEIGHT;
        }
    }

    return EAS_SUCC;
}

void AddrProxy::doFinish(bool bFail)
{
    ++_totalInvoke;
    time_t t = TNOW;
    TC_LockT<TC_ThreadMutex> lock(_mutex);
    if (bFail)
    {
        //调用失败
        //失败次数+1
        ++_timeoutInvoke;

        //连续失败时间间隔重新计算
        if (0 == _frequenceFailInvoke)
        {
            _frequenceFailTime = t + minFrequenceFailTime;
        }
        //连续失败次数加1
        _frequenceFailInvoke++;

        //检查是否到了连续失败次数,且至少在5s以上
        if (_frequenceFailInvoke >= frequenceFailInvoke && t >= _frequenceFailTime)
        {
            setTimeout(true);
            TLOGERROR( _upstream << "," << _addr
                      << ",disable frequenceFail,freqtimeout:" << _frequenceFailInvoke
                      << ",timeout:" << _timeoutInvoke
                      << ",total:" << _totalInvoke << "] " << endl);
            return ;
        }
    }
    else
    {
        _frequenceFailInvoke = 0;
        setTimeout(false);
    }

    //判断一段时间内的超时比例
    if (t > _nextFinishInvokeTime)
    {
        _nextFinishInvokeTime = t + checkTimeoutInterval;

        if (bFail && _timeoutInvoke >= minTimeoutInvoke && _timeoutInvoke*100 >= radio_100 * _totalInvoke)
        {
            setTimeout(true);
            TLOGERROR( _upstream << "," << _addr
                      << ",disable radioFail,freqtimeout:" << _frequenceFailInvoke
                      << ",timeout:" << _timeoutInvoke
                      << ",total:" << _totalInvoke << "] " << endl);
        }
        else
        {
            //每一分钟清空一次
            _totalInvoke = 0;
            _timeoutInvoke = 0;
            setTimeout(false);
        }
    }
}

bool AddrProxy::check()
{
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        if (_isActive || (!_isValid))
        {
            return true;
        }
    }

    TLOGDEBUG(getID() << endl);
    time_t t = TNOW;
    if (t < _nextCheckTime)
    {
        return false;
    }
    else
    {
        if (_lastInterval < 10)
        {
            _lastInterval += 1;
        }
        else if (_lastInterval < 120)
        {
            _lastInterval += 10;
        }

        _nextCheckTime = t + _lastInterval;
    }

    // do check...
    string monitorUrl = StationManager::getInstance()->getMonitorUrl(_upstream);
    bool ret = false;
    if (monitorUrl.empty())
    {
        ret = doTcpMonitor(_addr);
    }
    else
    {
        ret = doHttpMonitor(monitorUrl);
    }

    if (ret)
    {
        setActive(true);
    }

    return ret;
}

bool AddrProxy::checkConnect(const string& host, uint16_t port, int timeout)
{
    int fd; 
    struct sockaddr_in addr;
    struct timeval timeo = {timeout, 0}; 
    socklen_t len = sizeof(timeo);

    fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    addr.sin_port = htons(port);

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
    {
        if (errno == EINPROGRESS) 
        {
            TLOGDEBUG("connect " << host << ":" << port << " timeout." << endl);
        }
        else
        {
            TLOGDEBUG(host << ":" << port << " connect error:" << errno << ", " << strerror(errno) << endl);
        }

        return false;
    }

    TLOGDEBUG(host << ":" << port << " connect succ." << endl);

    return true;    
}

bool AddrProxy::doTcpMonitor(const string& addr)
{
    vector<string> v = TC_Common::sepstr<string>(addr, ":");
    if (v.size() != 2)
    {
        return false;
    }

    string host = v[0];
    uint16_t port = TC_Common::strto<uint16_t>(v[1]);

    bool ret = checkConnect(host, port, 3);
    return ret;
}

bool AddrProxy::doHttpMonitor(const string& url)
{
    try
    {
        string::size_type pos1 = 7;
        if (url.substr(0, 7) != "http://")
        {
            pos1 = 0;
        }

        string::size_type pos2 = url.find("/", pos1);
        if (pos2 == string::npos)
        {
            TLOGERROR("parse monitor url error:" << url << endl);
            return doTcpMonitor(_addr);
        }

        string host = url.substr(pos1, pos2 - pos1);
        string reqUrl = "http://" + _addr + url.substr(pos2);
        
        TC_HttpRequest stHttpReq;
        stHttpReq.setCacheControl("no-cache");
        stHttpReq.setUserAgent("Mozilla/4.0 (TupProxy Monitor Check)");
        stHttpReq.setGetRequest(reqUrl);
        stHttpReq.setHost(host);

        TC_HttpResponse stHttpRsp;
        int iRet = stHttpReq.doRequest(stHttpRsp, 3000);
        if(iRet == 0 && stHttpRsp.getStatus() == 200)
        {
            TLOGDEBUG(_upstream << "|" << reqUrl << " do monitor succ." << endl);
            return true;
        }
        else
        {
            TLOGDEBUG(_upstream << "|" << reqUrl << "|" << iRet << "|" << stHttpRsp.getStatus() << endl);
            return false;
        }
    }
    catch(const std::exception& e)
    {
        TLOGERROR(url << ", exception:" << e.what() << endl);
    }
    return false;
}

HttpProxy::HttpProxy(const string& upstream)
{
    //_addrUpdateTime = 0;
    //_addrVer = 0;
    //_validIndex = 0;
    _upstream = upstream;
    
    //setAddr();
}

HttpProxy::~HttpProxy()
{
    TC_LockT<TC_ThreadMutex> lock(*this);
    for (auto it = _addrPrxList.begin(); it != _addrPrxList.end(); ++it)
    {
        it->second->destroy();
    }
    TLOGERROR(_upstream << " HttpProxy destroy." << endl);
}

void HttpProxy::setAddr(const vector<UpstreamInfo>& vs, const string& ver)
{
    TLOGDEBUG(vs.size() << "|" << ver << "|" << _addrVer << endl);
    if (ver == _addrVer)
    {
        return;
    }
    // 这里不管vs是否为空都swap掉，因为可能同时下线掉所有的addr
    TC_LockT<TC_ThreadMutex> lock(*this);
    vector<string> addrList;
    for (size_t i = 0; i < vs.size(); i++)
    {
        if (_addrPrxList.find(vs[i].addr) == _addrPrxList.end())
        {
            AddrPrx prx = new AddrProxy(_upstream, vs[i].addr, vs[i].weight, vs[i].fusingOnOff);
            _addrPrxList[vs[i].addr] = prx;
            FDLOG("load") << FILE_FUNC_LINE << "add_proxy|" << _upstream << "|" << vs[i].addr << "|" << vs[i].weight << "|" << vs[i].fusingOnOff << endl;
        }
        else
        {
            _addrPrxList[vs[i].addr]->update(vs[i].weight, vs[i].fusingOnOff);
        }

        addrList.push_back(vs[i].addr);
    }

    for (size_t i = 0; i < _addrList.size(); ++i)
    {
        if (find(addrList.begin(), addrList.end(), _addrList[i]) == addrList.end())
        {
            if (_addrPrxList[_addrList[i]])
            {
                _addrPrxList[_addrList[i]]->destroy();
            }
            _addrPrxList.erase(_addrList[i]);
            FDLOG("load") << FILE_FUNC_LINE << "del_proxy|" << _upstream << "|" << _addrList[i] << endl;
        }
    }
    _addrList = addrList;
    // 如果地址列表发生变更，那么当前迭代器重置
    _prxIterator = _addrPrxList.begin();    
}

AddrPrx HttpProxy::getProxy()
{
    AddrPrx prx;
    
    TC_LockT<TC_ThreadMutex> lock(*this);
    size_t total = _addrPrxList.size();
    if (total == 1)
    {
        TLOGDEBUG("only one addr proxy, just return!" << endl);
        return _addrPrxList.begin()->second;
    }

    AddrProxy::E_ADDR_STATUS ret = AddrProxy::EAS_FAIL;
    for (size_t i = 0; i < total; i++)
    {
        if (_prxIterator == _addrPrxList.end())
        {
            _prxIterator = _addrPrxList.begin();
        }

        AddrProxy::E_ADDR_STATUS x = _prxIterator->second->available();
        TLOGDEBUG("select_proxy[" << i << "/" << total << "]" << _prxIterator->second->getID() << "|" << x << endl);
        if (AddrProxy::EAS_SUCC == x)
        {
            prx = _prxIterator->second;
            //_prxIterator++;
            return prx;
        }
        else if (x < ret)
        {
            prx = _prxIterator->second;
            ret = x;
        }
        _prxIterator++;
    }

    if (prx)
    {
        prx->incCount();
        TLOGDEBUG("select_proxy return proxy:" << prx->getID() << endl);
    }

    return prx;
}

HttpPrx HttpProxyFactory:: getHttpProxy(const string& upstream)
{
    TC_LockT<TC_ThreadRecMutex> lock(*this);
    auto it = _proxy.find(upstream);
	if(it != _proxy.end())
    {
        return it->second;
    }

    HttpPrx prx(new HttpProxy(upstream));
    _proxy[upstream] = prx;
    return prx;
}

void AddrCheckThread::run()
{
    while(!_terminate)
    {
        try
        {
            size_t pos;
            {
                TC_LockT<TC_ThreadMutex> lock(_mutex);
                pos = _index;
                _index = (_index + 1) % 2;
            }

            for (auto it = _addr[pos].begin(); it != _addr[pos].end();)
            {
                if (it->second->check())
                {
                    _addr[pos].erase(it++);
                }
                else
                {
                    it++;
                }
            }
        }
        catch(exception &ex)
        {
            TLOGERROR("exception:" << ex.what() << endl);
        }
        catch(...)
        {
            TLOGERROR("exception unknown error." << endl);
        }

        TC_ThreadLock::Lock lock(*this);
        timedWait(1000 * 60);
    }        
}