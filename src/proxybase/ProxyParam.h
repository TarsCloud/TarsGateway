#ifndef _PROXYPARAM_H_
#define _PROXYPARAM_H_
#include "servant/Application.h"
#include "ReportHelper.h"
#include "GatewayServer.h"
#include "Verify.h"

using namespace tars;

enum E_PROXY_TYPE
{
    EPT_CROSSDOMAIN = 0,
    EPT_MONITOR=1,
    EPT_TUP_PROXY=2,
    EPT_JSON_PROXY=3,
    EPT_HTTP_PROXY=4,
    EPT_ERROR_PROXY=10,
    EPT_OPTIONS_REQ = 11,
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

struct THashInfo
{
    enum E_HASH_TYPE
    {
        EHT_ROBINROUND = 0,
        EHT_REQUESTID = 1,
        EHT_HTTPHEAD = 2,
        EHT_CLIENTIP = 3,
        EHT_DEFAULT = 99,
    };
    E_HASH_TYPE type {EHT_DEFAULT};
    string httpHeadKey;
};

struct VerifyInfo
{
    string  tokenHeader;
    Base::VerifyPrx prx {NULL};
    vector<string>  verifyHeaders;
    bool    verifyBody {false};
};

struct ProxyExInfo
{
    THashInfo   hashInfo;
    VerifyInfo  verifyInfo;
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


