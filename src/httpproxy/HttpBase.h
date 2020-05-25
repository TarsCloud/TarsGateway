#ifndef _HTTPBASE_H_
#define _HTTPBASE_H_

#include "util/tc_http_async.h"
#include "proxybase/ProxyParam.h"
#include "HttpProxy.h"

using namespace tars;

class HttpBase 
{

public:
    HttpBase ();
    ~HttpBase ();

    void initializeHttp();

    int handleHttpRequest(HandleParam& param, shared_ptr<AccessLog> aLog);

    //int handleRequest(HandleParam& param, shared_ptr<AccessLog> aLog);

  protected:
    TC_HttpAsync _httpAsync;

    unordered_map<string, HttpPrx> _httpPrx;
};


#endif