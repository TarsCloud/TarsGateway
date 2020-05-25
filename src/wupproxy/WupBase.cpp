#include "WupBase.h"
#include "servant/Application.h"
#include "util/tc_http.h"
#include "util/tc_mysql.h"
#include "util/tc_parsepara.h"
#include "util/tc_config.h"
#include "util/tc_hash_fun.h"
#include "wup-linux-c++/wup.h"
//#include "WupProxyManager.h"
#include <zlib.h>
#include "util/tc_tea.h"
#include "util/tc_gzip.h"
#include "util/tc_parsepara.h"
#include "util/tc_config.h"
#include "util/tc_hash_fun.h"
#include "proxybase/ReportHelper.h"
#include "proxybase/ProxyUtils.h"
#include "WupProxyServer.h"
#include "httpproxy/StationManager.h"
#include "FlowControlManager.h"

//////////////////////////////////////////////////////

using namespace std;
using namespace wup;
using namespace tars;

std::atomic<int> g_requestId;
const string CTX_TARSHASH_KEY = "CTX_TARSHASH_KEY";

//////////////////////////////////////////////////////
//string WupBase::_LocalServerName = "Base.WupProxyServer";;
//unsigned int WupBase::_rspSizeLimit = 1048576;
int WupBase::_setConnectionTag = 0;

string WupBase::_sEncryptKey;
string WupBase::_sEncryptKeyV2;
//bool WupBase::_bReportMetis= true;
int WupBase::_iMinCompressLen = 512;
//map<string,bool> WupBase::_mCryptServantNames;
//bool WupBase::_bCryptCheck= false;

//////////////////////////////////////////////////////
WupBase::WupBase() //: _pEncryptBuff(NULL), _iEncryptLength(0)
{

}

WupBase::~WupBase()
{

}

void WupBase::initializeBase()
{
    loadFilterHeader();

    TLOGDEBUG("initializeBase OK!" << endl);
}

void WupBase::destroyBase()
{

    TLOGDEBUG("destroyBase OK!" << endl);
}

// 初始化静态配置
int WupBase::initStaticParam(const TC_Config& conf)
{
    //_LocalServerName = ServerConfig::Application + "." + ServerConfig::ServerName;

//    _sEncryptKey1    = "ABCDEFGHIJKLMNOP";
    _sEncryptKey    = "sDf434ol*123+-KD";
	_sEncryptKeyV2	= "lS-0O&62Jl(j%!zM";

    //是否设置回包的http包头Connection值为keep_alive,默认不设置
    _setConnectionTag = TC_Common::strto<int>(conf.get("/config/setConnection/<tag>", "0"));

    //默认是1M
    //_rspSizeLimit = TC_Common::strto<unsigned int>(conf.get("/config/limit/<rspsize>", "1048576"));

    return 0;
}

//////////////////////////////////////////////////////

bool WupBase::reloadFilterHeader(const string& command, const string& params, string& result)
{
    TLOGDEBUG("command:" << command << endl);
    result = loadFilterHeader();
    TLOGDEBUG("result:" << result << endl);
    return true;
}

string WupBase::loadFilterHeader()
{
    try
    {
        TC_Config conf;
        //conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
        conf.parseFile(g_app.getConfFile());
        _headers = TC_Common::sepstr<string>(conf.get("/main<filterheaders>", ""), ", ;|");

        _hashType = TC_Common::strto<int>(conf.get("/main/hash<hash_type>", "0"));

        _sHttpHeader = TC_Common::upper(conf.get("/main/hash<httpheader>"));

        TLOGDEBUG(TC_Common::tostr(_headers) << "|" << _hashType << "|" << _sHttpHeader << endl);

        return "reload ok";
    }
    catch (exception& ex)
    {
        TLOGERROR("error:" << ex.what() << endl);
        return string("loadFilterHeader config error:") + ex.what();
    }

    return "loadFilterHeader error.";
}

int WupBase::getHttpFilter(const TC_Http::http_header_type& filterHeader, map<string, string>& filters)
{
   vector<string>::iterator it = _headers.begin();
   while (it != _headers.end())
   {
       TC_Http::http_header_type::const_iterator i = filterHeader.find(*it);
       if (i != filterHeader.end())
       {
           filters[i->first] = i->second;
       }
       ++it;
   }

   return 0;
}

/** 
 * \
 * 
 * @param httpRequest
 * @param iZipType
 * @param iEptType
 * 
 * @return int
 */
int WupBase::getReqEncodingFromHeader(const TC_HttpRequest& httpRequest, int& iZipType, int& iEptType)
{
    // HTTP头规则
    //客户端设置：
    //X-S-ZIP：        自定义字段，标识post内容是否采用了gzip压缩。同时告诉服务端可接受压缩的response
    //X-R-ZIP：        自定义字段，告诉服务端可接受压缩的response。请求内容为压缩时可以不设置此头。
    //X-S-Encrypt：    自定义字段，标识post内容是否采用了加密，同时告诉服务端可接受加密的response

    //服务端：
    //X-S-ZIP：        自定义字段，标识post内容是否采用了gzip压缩
    //X-S-Encrypt：    自定义字段，标识response内容是否做了加密。

    // 交互流程
    //客户端发送请求时：
    //1、对准备发送的整个WUP包（对应HTTP包体）进行压缩。对应的需要置HTTP头：X-S-ZIP，标明本内容为压缩,同时可以接收压缩内容。
    //   X-S-ZIP:gzip
    //2、对压缩后的内容进行加密。对应的需要置HTTP头：X-S-Encrypt，标明本内容为加密，同时接收加密内容。
    //   X-S-Encrypt:qencrypt
    //   服务端为TEA算法，java代码可以使用jutil库
    //3、如果发送的内容不需要压缩，则要置HTTP头：X-R-ZIP，单独标明下可以接收压缩内容。
    //   X-R-ZIP:gzip
    //4、最终压缩及加密处理后的内容做为HTTP包体发送到服务端。
    //
    //  内容即要加密又要压缩时，要先压缩再加密。
    //
    //客户端收到应答时：
    //1、检测HTTP应答是否含有X-S-Encrypt头，如果有则对HTTP包体进行解密。
    //   X-S-Encrypt:encrypt1
    //2、检测HTTP应答是否含有X-S-ZIP头，如果有则对解密后的内容进行解压得到最终WUP包内容。如果没有，则1之后的内容就是最终WUP包内容。
    //   X-S-ZIP:gzip
    //
    // 内容即被加密又被压缩时，要先解密再解压。

    iZipType = 0;
    string sHeadList;
    //body内容是否是gzip格式
    std::string sTccEncoding = httpRequest.getHeader("X-S-ZIP");
    if (!sTccEncoding.empty())
    {
//        if (strcasestr(sTccEncoding.c_str(), "gzip") != NULL || sTccEncoding.find("xxxx") != string::npos)
        iZipType = 1;

        ReportHelper::reportProperty("HTTPHeader_X-S-ZIP");
        sHeadList += "X-S-ZIP;";
    }

    iEptType = 0;
    //body内容是否是加密格式
    std::string sTccEncrypt = httpRequest.getHeader("X-S-Encrypt");
	if (sTccEncrypt == "encrypt2")
	{
		iEptType = 2;

		ReportHelper::reportProperty("HTTPHeader_QQ-S-Encrypt");
		sHeadList += "X-S-Encrypt;";
	}
    else if (!sTccEncrypt.empty())
    {
//        if (sTccEncrypt.find("encrypt1") != string::npos)
        iEptType = 1;

        ReportHelper::reportProperty("HTTPHeader_QQ-S-Encrypt");
        sHeadList += "X-S-Encrypt;";
    }

    return 0;
}

int WupBase::setRspEncodingToHeader(const TC_HttpRequest& httpRequest, pair<string, string>& pairAcceptZip, pair<string, string>& pairAcceptEpt)
{
    string sHeadList;
    std::string sTccEncoding1 = httpRequest.getHeader("X-R-ZIP");
    std::string sTccEncoding2 = httpRequest.getHeader("X-S-ZIP");

    if (!sTccEncoding1.empty())
    {
        pairAcceptZip.first  = "X-S-ZIP";
        pairAcceptZip.second = "gzip";
        ReportHelper::reportProperty("HTTPHeader_X-R-ZIP");
        sHeadList += "X-R-ZIP;";
    }

    if (!sTccEncoding2.empty())
    {
        pairAcceptZip.first  = "X-S-ZIP";
        pairAcceptZip.second = "gzip";
        ReportHelper::reportProperty("HTTPHeader_X-S-ZIP");
        sHeadList += "X-S-ZIP;";
    }

    std::string sTccEncrypt2 = httpRequest.getHeader("X-S-Encrypt");
    if (!sTccEncrypt2.empty())
    {
        pairAcceptEpt.first  = "X-S-Encrypt";
        //pairAcceptEpt.second = "encrypt1";
		pairAcceptEpt.second = sTccEncrypt2;
        sHeadList += "X-S-Encrypt;";
    }

    TLOGDEBUG("Zip.first:"<< pairAcceptZip.first
        << ", Zip.second:"<< pairAcceptZip.second
        << ", Ept.first:" << pairAcceptEpt.first
        << ", Ept.second:" << pairAcceptEpt.second
        << ", HeaderList:" << sHeadList << endl);

    return 0;
}


int WupBase::getDataFromHTTPRequest(const TC_HttpRequest& httpRequest, vector<char>& buffer)//, size_t& iOrgReqLen)
{
    //根据Content-Length的长度来确定解包长度
    int contentLength = TC_Common::strto<int>(httpRequest.getHeader("Content-Length"), "0");
    if (contentLength > 0)
    {
        string tmp = httpRequest.getContent().substr(0, contentLength);

        buffer.assign(tmp.c_str(), tmp.c_str() + contentLength);
    }
    
    if (buffer.size() == 0)
    {
        TLOGDEBUG("buffer result is empty" << endl);
        ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "WupDataEmpyErr", -1);
        return 1;
    }
    else
    {
        return 0;
    }
}

int WupBase::getRealDataByDecode(vector<char>& sRealWupData, HandleParam& stParam)
{
    if (!stParam.iEptType && !stParam.iZipType)
    {
        return 0;
    }

    int iOrgLen = sRealWupData.size();

    vector<char> sTempData = sRealWupData;

    // 解密数据
    if (stParam.iEptType)
    {
        bool bEpt = TC_Tea::decrypt(stParam.sEncryptKey.c_str(), &sRealWupData[0], sRealWupData.size(), sTempData);
        if (!bEpt)
        {
            TLOGERROR("http protocol decrypt error, iOrgLen:" << iOrgLen << endl);
            ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "DecryptErr", -1);
            return -1;
        }

        TLOGDEBUG("TeaDecrypt OK, " << iOrgLen << "->" << sRealWupData.size() << endl);

        sTempData.swap(sRealWupData);
    }


    // 解压缩
    if (stParam.iZipType)
    {
        bool bGzip = TC_GZip::uncompress(&sRealWupData[0], sRealWupData.size(), sTempData);
        if (!bGzip)
        {
            TLOGERROR("http protocol gzip uncompress error, iOrgLen:" << iOrgLen << endl);
            ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "UncompressErr", -1);
            return -2;
        }

        sRealWupData.swap(sTempData);
        TLOGDEBUG("gzipUncompress OK, " << iOrgLen << "->" << sRealWupData.size() << endl);
    }

    return 0;
}

void WupBase::getFilter(HandleParam &stParam)
{
    getHttpFilter(stParam.httpRequest.getHeaders(), stParam.filterHeader);
    stParam.filterHeader["REMOTE_IP"] = stParam.sIP;
    stParam.filterHeader["X-Forwarded-For-Host"] = stParam.sIP + ":" + TC_Common::tostr(stParam.current->getPort());
    if (stParam.filterHeader.find("X-GUID") == stParam.filterHeader.end())
    {
        stParam.filterHeader["X-GUID"] = stParam.sGUID;
    }
    if (stParam.filterHeader.find("X-Forwarded-For-Host") == stParam.filterHeader.end())
    {
        stParam.filterHeader["X-Forwarded-For-Host"] = stParam.sIP; + ":" + TC_Common::tostr(stParam.current->getPort());
    }
}

int WupBase::handleTafRequest(HandleParam& stParam)
{
    string sErrMsg;

    try
    {
		stParam.current->setResponse(false);
        //解析出所有的wup请求
        BasePacket wupRequest;

		int ret = -1;
		if (EPT_JSON_PROXY == stParam.proxyType)
		{
			ret = parseJsonRequest(stParam, wupRequest);
		}
		else if(EPT_WUP_PROXY == stParam.proxyType)
		{
			ret = parseWupRequest(stParam, wupRequest);
		}

		if (ret != 0)
		{
			TLOGERROR("parseWupRequest error"
				<< ",ret:" << ret
				<< ",length:" << stParam.length
				<< ",sGUID:" << stParam.sGUID
				<< endl);

			ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "ParseWupBodyErr", -1);
			//stParam.current->close();
			ProxyUtils::doErrorRsp(400, stParam.current, stParam.httpKeepAlive);
			return 0;
		}

        //解析代理
        THashInfo hi;
        ServantPrx proxy = WupProxyManager::getInstance()->getProxy(wupRequest.sServantName, wupRequest.sFuncName, stParam.httpRequest, hi);

        if (!proxy)
        {
            TLOGERROR("wup request no match proxy error, "
                << ", sServantName:" << wupRequest.sServantName
                << ", sFuncName:" << wupRequest.sFuncName
                << ", sGUID:" << stParam.sGUID
                << endl);

            TLOGERROR("parseWupProxy error, sGUID:" << stParam.sGUID << endl);

            ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "GetProxyErr", -1);
			ProxyUtils::doErrorRsp(404, stParam.current, stParam.httpKeepAlive);
            return 0;
        }
        else
        {
            ReportHelper::reportStat(g_app.getLocalServerName(), "FindProxyMonitor", "WupProxyOK", 0);
        }
		
		if (!STATIONMNG->checkWhiteList(proxy->taf_name(), stParam.sIP))
        {
            TLOGERROR(stParam.sIP << " is not in " << proxy->taf_name() << " 's white list, obj:" << wupRequest.sServantName << ":" << wupRequest.sFuncName << endl);
            ProxyUtils::doErrorRsp(403, stParam.current, stParam.httpKeepAlive);
            return 0;
        }
        else if (STATIONMNG->isInBlackList(proxy->taf_name(), stParam.sIP))
        {
            TLOGERROR(stParam.sIP << " is in " << proxy->taf_name() << " 's black list, obj:" << wupRequest.sServantName << ":" << wupRequest.sFuncName << endl);
            ProxyUtils::doErrorRsp(403, stParam.current, stParam.httpKeepAlive);
            return 0;
        }
        else if (!FlowControlManager::getInstance()->check(proxy->taf_name()))
        {
            TLOGERROR("tars request:" << proxy->taf_name() << " flowcontrol false!!!" << endl);
            ProxyUtils::doErrorRsp(429, stParam.current, stParam.httpKeepAlive);
            return 0;
        }

        getFilter(stParam);

        //构造异步回调对象
        WupCallbackParam stCbParam;
        stCbParam.iTime = TC_Common::now2ms();
        stCbParam.pairAcceptZip = stParam.pairAcceptZip;
        stCbParam.pairAcceptEpt = stParam.pairAcceptEpt;
        //stCbParam.iPortType = stParam.iPortType;
        stCbParam.iEptType = stParam.iEptType;
        stCbParam.iZipType = stParam.iZipType;
        stCbParam.sReqGuid = stParam.sGUID;
        stCbParam.sReqXua  = stParam.sXUA;
        stCbParam.sReqIP = stParam.sIP;
		stCbParam.sEncryptKey = stParam.sEncryptKey;
		stCbParam.iReqBufferSize = stParam.length;

		stCbParam.iRequestId = wupRequest.iRequestId;
		stCbParam.sServantName = wupRequest.sServantName;
		stCbParam.sFuncName = wupRequest.sFuncName;
        stCbParam.httpKeepAlive = stParam.httpKeepAlive;
        stCbParam.isRestful = stParam.isRestful;

        WupCallbackPtr cb;
        if (JSONVERSION == wupRequest.iVersion)
        {
            cb = new WupCallback("json", stParam.current, stCbParam);
        }
        else if ( JCEVERSION == wupRequest.iVersion)
        {
            cb = new WupCallback("jce", stParam.current, stCbParam);
        }
        else
        {
            cb = new WupCallback("wup", stParam.current, stCbParam);
        }

        //记录日志
        FDLOG("wup") << 0 << "|"
            << stParam.sIP << "|"
            << stParam.sGUID << "|"
            << wupRequest.sServantName << "|"
			<< wupRequest.sFuncName << "|"
            << stParam.iEptType << "|"
            << stParam.iZipType << "|"
            << proxy->taf_name() << "|"
            //<< stParam.iPortType << "|"
//                << stParam.iOrgReqLen << "|"
            << stParam.length << "|"
            << endl;

        string sHttpHeaderValue;
        if (!hi.httpHeadKey.empty())
        {
            sHttpHeaderValue = stParam.httpRequest.getHeader(hi.httpHeadKey);
        }
        
        //设置需要透传的头部
        wupRequest.context.insert(stParam.filterHeader.begin(), stParam.filterHeader.end());
        // 正常用户
        wupAsyncCall(wupRequest, proxy, cb, hi.type, sHttpHeaderValue);

        return 0;
    }
    catch (exception& ex)
    {
        sErrMsg = ex.what();
    }
    catch (...)
    {
        sErrMsg = "unknown error";
    }
    //stParam.current->close();

	ProxyUtils::doErrorRsp(400, stParam.current, stParam.httpKeepAlive);

    TLOGERROR("exception: " << sErrMsg << ",sGUID:" << stParam.sGUID << endl);

    ReportHelper::reportStat(g_app.getLocalServerName(), "RequestMonitor", "ReqException2Num", -1);

    return -99;
}

int WupBase::parseJsonRequest(HandleParam& stParam, RequestPacket& wupRequest)
{
	try
	{
        string buff(stParam.buffer, stParam.buffer + stParam.length);

        wupRequest.iVersion = JSONVERSION;
        wupRequest.cPacketType = tars::JCENORMAL;

        if (stParam.httpRequest.getRequestUrl().length() > 6)
        {
            vector<string> vs = TC_Common::sepstr<string>(stParam.httpRequest.getRequestUrl(), "/");
            if (vs.size() < 3)
            {
                TLOGERROR("parse json url fail:" << stParam.httpRequest.getRequestUrl() << endl);
                return -1;
            }
            
            wupRequest.iRequestId = 99;
            wupRequest.sServantName = TC_Common::trim(vs[1]);
            wupRequest.sFuncName = TC_Common::trim(vs[2]);
            wupRequest.sBuffer.resize(buff.length());
            wupRequest.sBuffer.assign(buff.begin(), buff.end());
            stParam.isRestful = true;
        }
        else
        {
            
            TLOGDEBUG("buff:" << buff << endl);

            tars::JsonValuePtr p = tars::TC_Json::getValue(buff);
            tars::JsonValueObjPtr pObj = tars::JsonValueObjPtr::dynamicCast(p);
            tars::JsonInput::readJson(wupRequest.iMessageType, pObj->value["msgtype"], false);
            tars::JsonInput::readJson(wupRequest.iRequestId, pObj->value["reqid"], false);
            tars::JsonInput::readJson(wupRequest.iTimeout, pObj->value["timeout"], false);
            tars::JsonInput::readJson(wupRequest.context, pObj->value["context"], false);
            tars::JsonInput::readJson(wupRequest.status, pObj->value["status"], false);

            tars::JsonInput::readJson(wupRequest.sServantName, pObj->value["obj"], false);
            tars::JsonInput::readJson(wupRequest.sFuncName, pObj->value["func"], false);

            string data;
            tars::JsonInput::readJson(data, pObj->value["data"], true);
            if (data.empty())
            {
                TLOGERROR("parseJsonRequest error, data is empty!!!" << endl);
                return -1;
            }
            wupRequest.sBuffer.resize(data.length());
            wupRequest.sBuffer.assign(data.begin(), data.end());
            stParam.isRestful = false;
        }

		return 0;
	}
	catch (exception& ex)
	{
		TLOGERROR("exception:" << ex.what() << endl);
	}

	return -1;
}


int WupBase::parseWupRequest(HandleParam& stParam, RequestPacket& wupRequest) 
{
    const char *buffer = stParam.buffer;
    size_t length = stParam.length;
    //长度保护
	if (length < sizeof(uint32_t))
    {
        TLOGERROR("wup error: wup packet length < 4, length:" << length << endl);
        return -1;
    }

    uint32_t l = 0;
    memcpy(&l, buffer, sizeof(uint32_t));
    l = ntohl(l);

    //长度保护
    if (l > length)
    {
        TLOGERROR("wup error: " << l << " > " << length << endl);
        return -2;
    }

    if (l <= 4)
    {
        TLOGERROR("wup error: l:" << l << " <= 4, length:" << length << endl);
        return -3;
    }

    tars::JceInputStream<BufferReader> is;

    is.setBuffer(buffer + 4, l - 4);

	wupRequest.readFrom(is);

	TLOGDEBUG(wupRequest.sServantName << "::" << wupRequest.sFuncName << ", requestid:" << wupRequest.iRequestId << endl);

    return 0;
}

// ServantPrx WupBase::parseWupProxy(const BasePacket& wupRequest, HandleParam& stParam)
// {
// 	ServantPrx proxy = WupProxyManager::getInstance()->getProxy(wupRequest.sServantName, wupRequest.sFuncName, stParam.httpRequest);

// 	if (!proxy)
// 	{
// 		TLOGERROR("wup request no match proxy error, "
// 			<< ", sServantName:" << wupRequest.sServantName
// 			<< ", sFuncName:" << wupRequest.sFuncName
// 			<< ", sGUID:" << stParam.sGUID
// 			<< endl);

// 		ReportHelper::reportStat(g_app.getLocalServerName(), "FindProxyMonitor", "WupProxyErr", -1);
// 	}
// 	else
// 	{
// 		ReportHelper::reportStat(g_app.getLocalServerName(), "FindProxyMonitor", "WupProxyOK", 0);
// 	}

//     return proxy;
// }

void WupBase::wupAsyncCall(RequestPacket &wup, ServantPrx &proxy, const WupCallbackPtr &cb, THashInfo::E_HASH_TYPE ht, const string &sHttpHeaderValue)
{

    //用自己的requestid, 回来的时候好匹配
    int requestId = (++g_requestId);

    try
    {
        //后续需要修改requestid的值, 因此赋值一个wup

        //BasePacket newWup = wup;

        //设置新的wup requestid
        int oldReqID = wup.iRequestId;
        wup.iRequestId = requestId;

        //设置TARS服务的ServantName
        wup.sServantName = string(proxy->taf_name().substr(0, proxy->taf_name().find("@")));

        
        //wup.context.insert(filterHeader.begin(), filterHeader.end());
        //newWup.context = filterHeader;

        //重新编码
        tars::JceOutputStream<BufferWriter> buffer;

        wup.writeTo(buffer);

        unsigned int bufferlength = buffer.getLength() + 4;
        bufferlength = htonl(bufferlength);

        string s;
        s.append((char *)&bufferlength, 4);
        s.append(buffer.getBuffer(), buffer.getLength());

        cb->setNewRequestId(requestId);

        //异步发送出去(用requestId hash来发送)
		TLOGDEBUG("taf_name:" << proxy->taf_name() << endl);

        string sHashType;
        if (wup.context.find(CTX_TARSHASH_KEY) != wup.context.end() && wup.context[CTX_TARSHASH_KEY].length() > 0)
		{
			proxy->taf_hash(tars::hash<string>()(wup.context[CTX_TARSHASH_KEY]))->taf_rpc_call_async(requestId, wup.sFuncName,  s.c_str(), s.length(), cb);
			sHashType = "context hash, key=" + wup.context[CTX_TARSHASH_KEY];
		}
        else if (ht == THashInfo::EHT_ROBINROUND)
        {
            sHashType = "robin round";
            proxy->taf_rpc_call_async(requestId, wup.sFuncName, s.c_str(), s.length(), cb);
        }
        else if (ht == THashInfo::EHT_REQUESTID)
        {
            sHashType = "requestid hash, reqid=" + TC_Common::tostr(oldReqID);
            proxy->taf_hash(oldReqID)->taf_rpc_call_async(requestId, wup.sFuncName,  s.c_str(), s.length(), cb);
        }
        else if (ht == THashInfo::EHT_HTTPHEAD && !sHttpHeaderValue.empty())
        {
            sHashType = "http head hash, head=" + sHttpHeaderValue;
            proxy->taf_hash(tars::hash<string>()(sHttpHeaderValue))->taf_rpc_call_async(requestId, wup.sFuncName,  s.c_str(), s.length(), cb);
        }
        else if (ht == THashInfo::EHT_CLIENTIP)
        {
            sHashType = "clientip hash, ip=" + cb->getClientIp();
            proxy->taf_hash(tars::hash<string>()(cb->getClientIp()))->taf_rpc_call_async(requestId, wup.sFuncName,  s.c_str(), s.length(), cb);
        }
        else
        {
            sHashType = "default hash";
            proxy->taf_rpc_call_async(requestId, wup.sFuncName,  s.c_str(), s.length(), cb);
        }

		TLOGDEBUG(wup.sServantName << "::" << wup.sFuncName << ", version:" << (int)wup.iVersion << ", hash:" << sHashType << ", async call succ." << endl);

        FDLOG("call") << wup.sServantName + "::" + wup.sFuncName << "|" <<sHttpHeaderValue
            << "|iRequestId:" << wup.iRequestId
            << "|oldReqID:" << oldReqID
            << "|type:" << (int)(wup.iVersion)
            << "|buffer.size:" << s.length()
            << "|HashType:" << sHashType
            << "|ServantName:" << proxy->taf_name()
            << endl;
    }
    catch (exception& ex)
    {
        TLOGERROR("wupAsyncCall error:" << ex.what() << endl);
    }
}


//////////////////////////////////////////////////////

