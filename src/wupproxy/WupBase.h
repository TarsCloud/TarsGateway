#ifndef _WupBase_H_
#define _WupBase_H_

//////////////////////////////////////////////////////
#include "servant/Application.h"
#include "util/tc_common.h"
#include "tup/tup.h"
#include "proxybase/ProxyParam.h"
#include "WupCallback.h"
#include "WupProxyManager.h"
//#include "mtt_logger.h"
//#include "mtt_unireporter.h"
//////////////////////////////////////////////////////

using namespace tars;

//////////////////////////////////////////////////////

#define WUP_MREP_REQ_SIZE                                             "wup_req_size"                 // 请求包长度，解压后
#define WUP_MREP_RSP_SIZE                                             "wup_rsp_size"                 // 应答包长度，压缩前
#define WUP_MREP_REQ_TRUE_SIZE                                        "wup_reqtrue_size"             // 请求包长度，解压前
#define WUP_MREP_RSP_RUE_SIZE                                         "WupPackSize"                  // 应答包长度，压缩后

#define WUP_MREP_REQ_S_ZIP                                            "wup_req_s_zip"                // 请求内容为压缩的次数
#define WUP_MREP_REQ_R_ZIP                                            "wup_req_r_zip"                // 可以接收压缩的次数
#define WUP_MREP_REQ_S_CRYPT                                          "wup_req_s_crypt"              // 请求内容为加密的次数
#define WUP_MREP_REQ_R_CRYPT                                          "wup_req_r_crypt"              // 可以接收加密的次数

#define WUP_MREP_PARSE_ERR                                            "wup_parse_err"                // 解包失败
#define WUP_MREP_PROXY_ERR                                            "wup_proxy_err"                // 路由失败

//////////////////////////////////////////////////////
/**
* 基础、公用逻辑类 
*  
* 本类被回调类继续时，为减少开销可能不会初台化，要注意 
*/
class WupBase
{
public:
    WupBase();
    virtual ~WupBase();
    virtual void initializeBase();
    virtual void destroyBase();

    /**
     * 初始化静态配置
     */
    static int initStaticParam(const TC_Config& conf);

    /**
     * 重新加载过滤头
     * @param command
     * @param params
     * @param result
     * 
     * @return bool
     */
    bool reloadFilterHeader(const string& command, const string& params, string& result);

    /**
     * 获取透传的头部
     * @param filterHeader
     * 
     */
    int getHttpFilter(const TC_Http::http_header_type &filterHeader, map<string,string>& filters);

    /**
     * 加载过滤头
     * 
     * @return string
     */
    string loadFilterHeader();

    int getReqEncodingFromHeader(const TC_HttpRequest& httpRequest, int& iZipType, int& iEptType);
    
    int setRspEncodingToHeader(const TC_HttpRequest& httpRequest, pair<string, string>& pairAcceptZip, pair<string, string>& pairAcceptEpt);

protected:

    void getFilter(HandleParam &stParam);
  /**
     * 从HTTP POST Data或者GET参数中取出Wup数据
     * 
     */
  int getDataFromHTTPRequest(const TC_HttpRequest &httpRequest, vector<char> &buffer); //, size_t& iOrgReqLen );

  /**
     * 解压，解密数据
     * 
     */
  int getRealDataByDecode(vector<char> &sRealWupData, HandleParam &stParam);

  /**
     * WUP包具体处理
     * 
     * @param stParam 
     */
  int handleTafRequest(HandleParam &stParam);

  /**
     * 解析WUP包
     * 
     * @param buffer 
     * @param length 
     * @param wupRequest
     * @return int
     */
  int parseWupRequest(HandleParam &stParam, RequestPacket &wupRequest);

  /**
     * 解析JSON包
     * 
     * @param buffer 
     * @param length 
     * @param wupRequest
     * @return int
     */
  int parseJsonRequest(HandleParam &stParam, RequestPacket &wupRequest);

  /**
     * 解析所有的代理
     * 
     * @param mWupRequest 
     * @param map<int32_t, ServantPrx> 
     * @return int
     */
  //ServantPrx parseWupProxy(const BasePacket &wupRequest, HandleParam &stParam);

  /**
     * 异步发送
     * 
     * @param wup 
     * @param proxy 
     */
  void wupAsyncCall(RequestPacket &wup, ServantPrx &proxy, const WupCallbackPtr &cb, THashInfo::E_HASH_TYPE ht, const string &sHttpHeaderValue);

protected:
    // 参数类
    //static string              _LocalServerName;

    //static unsigned int        _rspSizeLimit;

    static int                 _setConnectionTag;

//    static bool                _userbaseSave;

//    static string              _sEncryptKey1;
    static string				_sEncryptKey;
	static string				_sEncryptKeyV2;

//    static bool                _bReportMetis;

    static int                 _iMinCompressLen;

//    static map<string,bool>    _mCryptServantNames;
//    static bool                _bCryptCheck;

protected:
    // 会写：重新加载
    vector<string>             _headers;
    int                        _hashType;
    string                     _sHttpHeader;
//    map<string, string>        _mDyeValue;
    string                     _crossDomain;

    // 数据类
//    char *                     _pEncryptBuff;
//    int                        _iEncryptLength;
};


/////////////////////////////////////////////////////
#endif

