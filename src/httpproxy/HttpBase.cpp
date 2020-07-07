#include "HttpBase.h"
#include "proxybase/FlowControlManager.h"
#include "HttpCallBack.h"
#include "HttpRouter.h"
#include "StationManager.h"
#include "proxybase/ProxyUtils.h"

HttpBase::HttpBase()
{
}

HttpBase::~HttpBase()
{
    _httpAsync.terminate();
}

void HttpBase::initializeHttp()
{
    _httpAsync.setTimeout(10000);
    _httpAsync.start();
}

/*
int HttpBase::handleHttpRequest(HandleParam& param, shared_ptr<AccessLog> aLog)
{
    string reqUrl = param.httpRequest.getOriginRequest();
    string::size_type pos = reqUrl.find("?");
    if (pos != string::npos)
    {
        reqUrl = reqUrl.substr(0, pos);
    }
    aLog->reqUrl = reqUrl;

    TLOGDEBUG("requrl:" << reqUrl << endl);
    string &stationId = aLog->station;
    if (!ROUTERMNG->getStation(reqUrl, aLog->station, aLog->funcPath))
    {
        TLOGERROR("find station fail, url:" << param.httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "find station fail";
        aLog->status = 404;
        ProxyUtils::doErrorRsp(404, param.current, param.httpKeepAlive);
        return -1;
    }
    else
    {
        TLOGDEBUG("staion:" << stationId << ", url:" << reqUrl << endl);
    }

    if (STATIONMNG->isInBlackList(stationId, param.sIP))
    {
        TLOGERROR(param.sIP << " is in " << stationId << " 's black list, url:" << param.httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "in station blacklist";
        aLog->status = 403;
        ProxyUtils::doErrorRsp(403, param.current, param.httpKeepAlive);
        return -2; 
    }

    if (!FlowControlManager::getInstance()->check(stationId))
    {
        TLOGERROR("station:" << stationId << " flowcontrol false!!!" << endl);
        aLog->errorMsg = "flowcontrol";
        aLog->status = 503;
        ProxyUtils::doErrorRsp(503, param.current, param.httpKeepAlive);
        return -3;
    }

    if (_httpPrx.find(stationId) == _httpPrx.end()) 
    {
        _httpPrx[stationId] = HttpProxyFactory::getInstance()->getHttpProxy(stationId);
    }
    AddrPrx proxy = _httpPrx[stationId]->getProxy();
    if (!proxy)
    {
        TLOGERROR(stationId << " has no valid proxy!" << endl);
        aLog->errorMsg = "has no valid proxy";
        aLog->status = 501;
        ProxyUtils::doErrorRsp(501, param.current, param.httpKeepAlive);
        return -4;
    }
    else
    {
        TLOGDEBUG("select addr succ:" << stationId << "|" << proxy->getAddr() << "|" << reqUrl << endl);
    }

    // _httpAsync.setProxyAddr(proxy->getAddr().c_str());
    aLog->proxyAddr = proxy->getAddr();

    TC_HttpAsync::RequestCallbackPtr cb = new AsyncHttpCallback(reqUrl, param.current, proxy, aLog, param.httpKeepAlive);
    _httpAsync.doAsyncRequest(param.httpRequest, cb, proxy->getAddr());

    return 0;
}
*/

int HttpBase::handleHttpRequest(HandleParam &param, shared_ptr<AccessLog> aLog)
{
    aLog->reqUrl = param.httpRequest.getRequestUrl();
    string &reqUrl = aLog->reqUrl;

    TLOGDEBUG("requrl:" << reqUrl << endl);
    //string &stationId = aLog->station;
    RouterResult rr;
    if (!Router->parse(param.httpRequest.getURL().getDomain(), param.httpRequest.getRequestUrl(), rr))
    {
        TLOGERROR("find station fail, url:" << param.httpRequest.getRequestUrl() << ", host:" << param.httpRequest.getHost() << endl);
        aLog->errorMsg = "find station fail";
        aLog->status = 404;
        ProxyUtils::doErrorRsp(404, param.current, param.httpKeepAlive);
        return -1;
    }
    else
    {
        aLog->station = rr.stationId;
        aLog->funcPath = rr.funcPath;
        TLOGDEBUG("staion:" << rr.stationId << ", url:" << reqUrl << ", new path:" << rr.path << endl);
    }

    if (!STATIONMNG->checkWhiteList(rr.stationId, param.sIP))
    {
        TLOGERROR(param.sIP << " is not in " << rr.stationId << " 's white list, url:" << param.httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "not in station whitelist";
        aLog->status = 403;
        ProxyUtils::doErrorRsp(403, param.current, param.httpKeepAlive);
        return -2;
    }

    if (STATIONMNG->isInBlackList(rr.stationId, param.sIP))
    {
        TLOGERROR(param.sIP << " is in " << rr.stationId << " 's black list, url:" << param.httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "in station blacklist";
        aLog->status = 403;
        ProxyUtils::doErrorRsp(403, param.current, param.httpKeepAlive);
        return -2;
    }

    if (!FlowControlManager::getInstance()->check(rr.stationId))
    {
        TLOGERROR("station:" << rr.stationId << " flowcontrol false!!!" << endl);
        aLog->errorMsg = "flowcontrol";
        aLog->status = 429;
        ProxyUtils::doErrorRsp(aLog->status, param.current, param.httpKeepAlive);
        return -3;
    }

    AddrPrx proxy = NULL;
    if (rr.upstream.find(":") != string::npos)
    {
        aLog->proxyAddr = rr.upstream;
        TLOGDEBUG("using proxypass upstream, addr:" << aLog->proxyAddr << endl);
    }
    else
    {
        if (_httpPrx.find(rr.upstream) == _httpPrx.end())
        {
            _httpPrx[rr.upstream] = HttpProxyFactory::getInstance()->getHttpProxy(rr.upstream);
        }
        proxy = _httpPrx[rr.upstream]->getProxy();
        if (!proxy)
        {
            TLOGERROR(rr.upstream << " has no valid proxy!" << endl);
            aLog->errorMsg = "has no valid proxy";
            aLog->status = 500;
            ProxyUtils::doErrorRsp(500, param.current, param.httpKeepAlive);
            return -4;
        }
        else
        {
            aLog->proxyAddr = proxy->getAddr();
            TLOGDEBUG("select addr succ:" << rr.upstream << "|" << proxy->getAddr() << "|" << reqUrl << endl);
        }
    }

    param.httpRequest.setPath(rr.path.c_str());
    TC_HttpAsync::RequestCallbackPtr cb = new AsyncHttpCallback(reqUrl, param.current, proxy, aLog, param.httpKeepAlive);
    _httpAsync.doAsyncRequest(param.httpRequest, cb, aLog->proxyAddr);
    TLOGDEBUG("doAsyncHttpRequest, " << aLog->host << "/" << reqUrl << "=>" << aLog->proxyAddr << endl);
    return 0;
}