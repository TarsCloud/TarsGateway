#ifndef _PROXYPARAM_H_
#define _PROXYPARAM_H_
#include "servant/Application.h"
#include "ReportHelper.h"
#include "GetwayServer.h"

using namespace tars;

enum E_PROXY_TYPE
{
    EPT_CROSSDOMAIN,
    EPT_MONITOR,
    EPT_WUP_PROXY,
    EPT_JSON_PROXY,
    EPT_HTTP_PROXY,
};

enum E_HTTP_RESLUT
{
    EHR_SUCC = 0,
    EHR_TIMEOUT = 1,
    EHR_CONNRRR = 2,
};

struct HandleParam
{
    tars::TarsCurrentPtr       current;
    const char *             buffer;
    size_t                   length;
    map<string,string>       filterHeader;
    TC_HttpRequest           httpRequest;
    pair<string, string>     pairAcceptZip;
    pair<string, string>     pairAcceptEpt;
    string                   sIP;
    string                   sGUID;
    string                   sXUA;
    int                      iEptType;
    int                      iZipType;
    string					 sEncryptKey;
    E_PROXY_TYPE             proxyType;
    bool                     httpKeepAlive;
    bool                     isRestful;
};

struct UpstreamInfo
{
    string  addr;
    int     weight;
    bool    fusingOnOff;
};

struct AccessLog
{
    string  clientIp;
    string  accessTime;
    string  rspTime;
    unsigned int  costTime;
    string  host;
    string  referer;
    size_t reqSize;
    string ua;
    string  httpMethod;
    string  reqUrl;
    string station;
    string funcPath;
    string proxyAddr;
    int  status;
    size_t  rspSize;
    string errorMsg;
    uint64_t accessMS;
    int iRet;
    bool needDisplay;

    AccessLog()
    {
        accessTime = TC_Common::now2str();
        accessMS = TC_Common::now2ms();
        costTime = 0;
        status = 200;
        iRet = -1;
        needDisplay = true;
    }

    ~AccessLog()
    {
        display();
        report();
    }

    void display()
    {
        if (0 == costTime)
        {
            costTime = TC_Common::now2ms() - accessMS;
            rspTime = TC_Common::now2str();
        }
        
        // .js 和 .css 文件请求，这里不打印accesslog， 不然太多
        if ( (reqUrl.length() > 4 && 0 == strncasecmp(".css", reqUrl.c_str()+reqUrl.length()-4, 4)) 
            || (reqUrl.length() > 3 && 0 == strncasecmp(".js", reqUrl.c_str()+reqUrl.length()-3, 3) ) )
        {
            needDisplay = false;
            return;
        }

        FDLOG("access") << clientIp << "|"
                        << accessTime << "|"
                        << host << "|"
                        << referer << "|"
                        << reqUrl << "|"
                        << reqSize << "|"
                        << httpMethod << "|"
                        << station << "|"
                        << proxyAddr << "|"
                        << status << "|"
                        << rspTime << "|"
                        << costTime << "|"
                        << rspSize << "|"
                        << ua << "|"
                        << errorMsg << endl;
    }

    void report()
    {
        if (0 == costTime)
        {
            costTime = TC_Common::now2ms() - accessMS;
            rspTime = TC_Common::now2str();
        }
        
        string proxyIP = proxyAddr.substr(0, proxyAddr.find(":"));
        ReportHelper::reportStat(g_app.getLocalServerName(), "http_" + station, funcPath, iRet, costTime, proxyIP);
        ReportHelper::reportProperty("http_" + station + "_" + TC_Common::tostr(status));
    }
};

#endif // !1__PROXYPARAM_H_


