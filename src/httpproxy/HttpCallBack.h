#ifndef _HTTPCALLBACK_H_
#define _HTTPCALLBACK_H_

#include "util/tc_http_async.h"
#include "servant/Application.h"
#include "HttpProxy.h"
#include "proxybase/ProxyParam.h"

using namespace std;
using namespace tars;

class AsyncHttpCallback : public TC_HttpAsync::RequestCallback
{
public:
    AsyncHttpCallback(const string &url, tars::TarsCurrentPtr current, AddrPrx addrPrx, shared_ptr<AccessLog> aLog, bool keepalive) 
    : _url(url), _current(current), _addrPrx(addrPrx), _aLog(aLog), _keepalive(keepalive)
    {
        // _url = url;
        // _current = current;
        // _addrPrx = addrPrx;
        // _aLog = aLog;
    }

    virtual void onSucc(TC_HttpResponse &stHttpResponse);

    virtual bool onContinue(TC_HttpResponse &stHttpResponse);

    virtual void onFailed(FAILED_CODE ret, const string &info);

    virtual void onClose();

protected:
    string _url;
    tars::TarsCurrentPtr _current;
    AddrPrx _addrPrx;
    shared_ptr<AccessLog> _aLog;
    bool _keepalive;
};

#endif // !1 _HTTPCALLBACK_H_
