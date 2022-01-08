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

int HttpBase::handleHttpRequest(shared_ptr<HandleParam> param, shared_ptr<AccessLog> aLog)
{
    aLog->reqUrl = param->httpRequest.getRequestUrl();
    string &reqUrl = aLog->reqUrl;

    TLOG_DEBUG("requrl:" << reqUrl << endl);
    //string &stationId = aLog->station;
    RouterResult rr;
    if (!Router->parse(param->httpRequest.getURL().getDomain(), param->httpRequest.getRequestUrl(), rr))
    {
        TLOG_ERROR("find station fail, url:" << param->httpRequest.getRequestUrl() << ", host:" << param->httpRequest.getHost() << endl);
        aLog->errorMsg = "find station fail";
        aLog->status = 404;
        ProxyUtils::doErrorRsp(404, param->current, param->proxyType, param->httpKeepAlive);
        return -1;
    }
    else
    {
        aLog->station = rr.stationId;
        aLog->funcPath = rr.funcPath;
        TLOG_DEBUG("staion:" << rr.stationId << ", url:" << reqUrl << ", new path:" << rr.path << endl);
    }

    if (!STATIONMNG->checkWhiteList(rr.stationId, param->sIP))
    {
        TLOG_ERROR(param->sIP << " is not in " << rr.stationId << " 's white list, url:" << param->httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "not in station whitelist";
        aLog->status = 403;
        ProxyUtils::doErrorRsp(403,  param->current, param->proxyType,param->httpKeepAlive);
        return -2;
    }

    if (STATIONMNG->isInBlackList(rr.stationId, param->sIP))
    {
        TLOG_ERROR(param->sIP << " is in " << rr.stationId << " 's black list, url:" << param->httpRequest.getRequestUrl() << endl);
        aLog->errorMsg = "in station blacklist";
        aLog->status = 403;
        ProxyUtils::doErrorRsp(403, param->current, param->proxyType, param->httpKeepAlive);
        return -2;
    }

    if (!FlowControlManager::getInstance()->check(rr.stationId))
    {
        TLOG_ERROR("station:" << rr.stationId << " flowcontrol false!!!" << endl);
        aLog->errorMsg = "flowcontrol";
        aLog->status = 429;
        ProxyUtils::doErrorRsp(aLog->status,  param->current, param->proxyType,param->httpKeepAlive);
        return -3;
    }

    AddrPrx proxy = NULL;
    if (rr.upstream.find(":") != string::npos)
    {
        aLog->proxyAddr = rr.upstream;
        TLOG_DEBUG("using proxypass upstream, addr:" << aLog->proxyAddr << endl);
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
            TLOG_ERROR(rr.upstream << " has no valid proxy!" << endl);
            aLog->errorMsg = "has no valid proxy";
            aLog->status = 500;
            ProxyUtils::doErrorRsp(500,  param->current, param->proxyType,param->httpKeepAlive);
            return -4;
        }
        else
        {
            aLog->proxyAddr = proxy->getAddr();
            TLOG_DEBUG("select addr succ:" << rr.upstream << "|" << proxy->getAddr() << "|" << reqUrl << endl);
        }
    }

    param->httpRequest.setPath(rr.path.c_str());
    TC_HttpAsync::RequestCallbackPtr cb = new AsyncHttpCallback(reqUrl, param->current, proxy, aLog, param->httpKeepAlive);
    _httpAsync.doAsyncRequest(param->httpRequest, cb, aLog->proxyAddr);
    TLOG_DEBUG("doAsyncHttpRequest, " << aLog->host << "/" << reqUrl << "=>" << aLog->proxyAddr << endl);
    return 0;
}