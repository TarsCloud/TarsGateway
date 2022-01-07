#include "ProxyImp.h"
#include "servant/Application.h"
#include "servant/TarsLogger.h"
#include "tup/tup.h"
#include "util/tc_http.h"
//#include "util/tc_atomic.h"
#include "GatewayServer.h"
#include "httpproxy/StationManager.h"
#include "proxybase/ProxyUtils.h"
#include "proxybase/ReportHelper.h"
#include "tupproxy/TupProxyManager.h"
#include "util/tc_base64.h"
#include <atomic>

//////////////////////////////////////////////////////
using namespace std;
using namespace tup;

//////////////////////////////////////////////////////

//////////////////////////////////////////////////////
ProxyImp::ProxyImp()
{
}

ProxyImp::~ProxyImp()
{
}

//////////////////////////////////////////////////////

void ProxyImp::initialize()
{
    initializeBase();
    initializeHttp();

    TARS_ADD_ADMIN_CMD_NORMAL("loadFilterHeader", TupBase::reloadFilterHeader);
}

void ProxyImp::destroy()
{
    destroyBase();
}

//////////////////////////////////////////////////////

int ProxyImp::doRequest(tars::TarsCurrentPtr current, vector<char> &response)
{
    shared_ptr<HandleParam> stParam = make_shared<HandleParam>();;
    string sErrMsg;

    try
    {
        const vector<char> &request = current->getRequestBuffer();
        ReportHelper::reportProperty("TupTotalReqNum");

        //string sReqContent(&request[0], request.size());

        stParam->current = current;
        stParam->buffer = NULL;
        stParam->length = 0;
        stParam->iEptType = 0;
        stParam->iZipType = 0;

        stParam->httpRequest.decode(&request[0], request.size());

        TLOG_DEBUG("method:" << stParam->httpRequest.getMethod() << "\r\n, request header:\r\n"
                  << stParam->httpRequest.genHeader() << endl);


        string sRemoteIp; //= stParam->httpRequest.getHeader("X-Forwarded-For-Pound");

        // 优先从http头中获取透传的ip。
        // if (sRemoteIp.empty())
        if (stParam->httpRequest.hasHeader("X-Forwarded-For-Pound"))
        {
            sRemoteIp = stParam->httpRequest.getHeader("X-Forwarded-For");
        }
        // 如果没有http透传ip，那么直接从current里面获取。
        if (sRemoteIp.empty())
        {
            sRemoteIp = current->getIp();
        }

        TLOG_DEBUG("sRemoteIp:" << sRemoteIp << endl);

        if (stParam->httpRequest.checkHeader("Connection", "keep-alive"))
        {
            stParam->httpKeepAlive = true;
        }
        else
        {
            stParam->httpKeepAlive = false;
        }

        if (STATIONMNG->isInBlackList("", sRemoteIp))
        {
            TLOG_ERROR(sRemoteIp << " is in Global black list, url:" << stParam->httpRequest.getRequestUrl() << endl);
            ProxyUtils::doErrorRsp(403, stParam->current, stParam->httpKeepAlive);
            return -1;
        }

        //跨域请求, 浏览器发起的飞行预检
        if (stParam->httpRequest.isOPTIONS())
        {
            ProxyUtils::doOptionsRsp(current, stParam->httpKeepAlive);
            TLOG_DEBUG("doOptionsRsp:" << stParam->httpRequest.getRequestUrl() << endl);
            return 0;
        }

        stParam->sIP = sRemoteIp;
        //stParam->filterHeader["REMOTE_IP"] = sRemoteIp;
        //stParam->filterHeader["X-Forwarded-For-Host"] = sRemoteIp + ":" + TC_Common::tostr(current->getPort());
        // 相关头信息提取
        stParam->sGUID = TC_Common::lower(stParam->httpRequest.getHeader("X-GUID"));
        stParam->sXUA = stParam->httpRequest.getHeader("X-XUA");
        //stParam->filterHeader["X-GUID"] = stParam->sGUID;

        //如果服务器返回的响应头信息中包含Expect: 100-continue，则表示 Server 愿意接受数据，这时才 POST 真正数据给 Server；
        //因为http异步不支行长连接,所以在收到upstream返回http/1.1 100后会断开连接，upstream接收不到真正的数据。
        //对于带了真正数据又带了Expect头的情况，去掉Expect头可以解决。
        if(stParam->httpRequest.hasHeader("Expect"))
        {
            stParam->httpRequest.eraseHeader("Expect");
        }

        stParam->proxyType = parseReqType(stParam->httpRequest.getRequestUrl(), stParam->httpRequest.getURL().getDomain());

        // 统一设置不自动回包
        current->setResponse(false);

        ///过滤自动监控测试请求
        if (EPT_CROSSDOMAIN == stParam->proxyType || EPT_MONITOR == stParam->proxyType)
        {
            // 是监控请求，内部已经返回应答
            filterMonitor(stParam);
            return 0;
        }
        else if (EPT_HTTP_PROXY == stParam->proxyType)
        {
            shared_ptr<AccessLog> aLog(new AccessLog());
            aLog->accessTime = TC_Common::now2str();
            aLog->clientIp = sRemoteIp;
            aLog->host = stParam->httpRequest.getHost();
            aLog->httpMethod = stParam->httpRequest.getMethod(); //isGET() ? "GET" : (stParam->httpRequest.isPOST() ? "POST" : (stParam->httpRequest.isOPTIONS() ? "OPTIONS" : "OTHER"));
            aLog->referer = stParam->httpRequest.getHeader("Referer");
            //aLog->reqUrl = stParam->httpRequest.
            aLog->reqSize = stParam->httpRequest.getContentLength();
            aLog->ua = stParam->httpRequest.getHeader("User-Agent");
            return handleHttpRequest(stParam, aLog);
        }

        /////////////////////////////////////////////////////////////////////////////////
        ///压缩，加密等HTTP头设置判断

        // 从头中获取请求内容是否加密，是否压缩
        getReqEncodingFromHeader(stParam->httpRequest, stParam->iZipType, stParam->iEptType);
        if (stParam->iEptType == 2)
        {
            stParam->sEncryptKey = _sEncryptKeyV2;
        }
        else if (stParam->iEptType == 1)
        {
            stParam->sEncryptKey = _sEncryptKey;
        }
        else
        {
            // 设置应答时使用的压缩及加密头
            setRspEncodingToHeader(stParam->httpRequest, stParam->pairAcceptZip, stParam->pairAcceptEpt);

            /// 从HTTP POST Data或者GET参数中取出Tup数据
            vector<char> sTupData;
            if (getDataFromHTTPRequest(stParam->httpRequest, sTupData) != 0)
            {
                TLOG_ERROR("getDataFromHTTPRequest failed"
                          << ",sGUID:" << stParam->sGUID << endl);
                //current->close();
                current->setResponse(false);
                ProxyUtils::doErrorRsp(400, current, stParam->httpKeepAlive);
                return 0;
            }

            /// 解压，解密数据
            if (getRealDataByDecode(sTupData, stParam) != 0)
            {
                TLOG_ERROR("getRealDataByDecode failed"
                          << ",sGUID:" << stParam->sGUID << endl);
                ProxyUtils::doErrorRsp(400, stParam->current, stParam->httpKeepAlive);
                //current->close();
                return 0;
            }

            stParam->buffer = &sTupData[0];
            stParam->length = sTupData.size();

            return handleTarsRequest(stParam);
        }
    }
    catch (exception &ex)
    {
        TLOG_ERROR("exception: " << ex.what() << ",sGUID:" << stParam->sGUID << endl);
    }
    catch (...)
    {
        TLOG_ERROR("exception: unknow exception, sGUID:" << stParam->sGUID << endl);
    }

    ProxyUtils::doErrorRsp(500, current, stParam->httpKeepAlive);
    ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "Exception1Num", -1);

    return 0;
}

E_PROXY_TYPE ProxyImp::parseReqType(const string &reqUrl, const string &host)
{
    E_PROXY_TYPE ret = EPT_HTTP_PROXY;
    if (!g_app.isTupHost(host))
    {
        ;
    }
    // else if ((reqUrl.empty() || reqUrl == "/"))
    // {
    //     // 兼容线上tup没有带路径的情况
    //     ret = EPT_TUP_PROXY;
    // }
    else
    {
        size_t l = reqUrl.length();
        if (l > 0 && reqUrl.back() == '/')
        {
            l--;
        }

        if (l == g_app.getTupPath().length() && strncmp(g_app.getTupPath().c_str(), reqUrl.c_str(), l) == 0)
        {
            ret = EPT_TUP_PROXY;
        }
        else if (l == g_app.getJsonPath().length() && strncmp(g_app.getJsonPath().c_str(), reqUrl.c_str(), l) == 0)
        {
            ret = EPT_JSON_PROXY;
        }
        else if (l > g_app.getJsonPathEx().length() && strncmp(g_app.getJsonPathEx().c_str(), reqUrl.c_str(), g_app.getJsonPathEx().length()) == 0)
        {
            ret = EPT_JSON_PROXY;
        }
        else if (l == g_app.getMonitorUrl().length() && strncmp(g_app.getMonitorUrl().c_str(), reqUrl.c_str(), l) == 0)
        {
            ret = EPT_MONITOR;
        }
    }

    TLOG_DEBUG(host << "|" << reqUrl << ", type:" << ret << endl);
    return ret;
}

void ProxyImp::filterMonitor(shared_ptr<HandleParam> stParam)
{
    if (EPT_MONITOR == stParam->proxyType) //监控用
    {

        TC_HttpResponse response;
        string data = "<html>hello GatewayMonitor! [version:1.2]</html>";
        response.setResponse(200, "OK", data);
        response.setContentType("text/html;charset=utf-8");
        response.setContentLength(data.length());
        if (stParam->httpKeepAlive)
        {
            response.setConnection("keep-alive");
        }
        else
        {
            response.setConnection("close");
        }
        g_app.setRspHeaders((int)EPT_MONITOR, "", response);
        //response.setConnection("close");
        string buffer = response.encode();

        stParam->current->sendResponse(buffer.c_str(), buffer.length());
        if (!stParam->httpKeepAlive)
        {
            stParam->current->close();
        }
        ReportHelper::reportProperty("TupMonitorNum");
    }
}