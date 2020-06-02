#ifndef _TupBase_H_
#define _TupBase_H_

//////////////////////////////////////////////////////
#include "servant/Application.h"
#include "util/tc_common.h"
#include "tup/tup.h"
#include "proxybase/ProxyParam.h"
#include "TupCallback.h"
#include "TupProxyManager.h"
//#include "mtt_logger.h"
//#include "mtt_unireporter.h"
//////////////////////////////////////////////////////

using namespace tars;

//////////////////////////////////////////////////////

#define TUP_MREP_REQ_SIZE                                             "tup_req_size"                 // 请求包长度，解压后
#define TUP_MREP_RSP_SIZE                                             "tup_rsp_size"                 // 应答包长度，压缩前
#define TUP_MREP_REQ_TRUE_SIZE                                        "tup_reqtrue_size"             // 请求包长度，解压前
#define TUP_MREP_RSP_RUE_SIZE                                         "TupPackSize"                  // 应答包长度，压缩后

#define TUP_MREP_REQ_S_ZIP                                            "tup_req_s_zip"                // 请求内容为压缩的次数
#define TUP_MREP_REQ_R_ZIP                                            "tup_req_r_zip"                // 可以接收压缩的次数
#define TUP_MREP_REQ_S_CRYPT                                          "tup_req_s_crypt"              // 请求内容为加密的次数
#define TUP_MREP_REQ_R_CRYPT                                          "tup_req_r_crypt"              // 可以接收加密的次数

#define TUP_MREP_PARSE_ERR                                            "tup_parse_err"                // 解包失败
#define TUP_MREP_PROXY_ERR                                            "tup_proxy_err"                // 路由失败

//////////////////////////////////////////////////////
/**
* 基础、公用逻辑类 
*  
* 本类被回调类继续时，为减少开销可能不会初台化，要注意 
*/
class TupBase
{
public:
    TupBase();
    virtual ~TupBase();
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
     * 从HTTP POST Data或者GET参数中取出Tup数据
     * 
     */
  int getDataFromHTTPRequest(const TC_HttpRequest &httpRequest, vector<char> &buffer); //, size_t& iOrgReqLen );

  /**
     * 解压，解密数据
     * 
     */
  int getRealDataByDecode(vector<char> &sRealTupData, HandleParam &stParam);

  /**
     * TUP包具体处理
     * 
     * @param stParam 
     */
  int handleTarsRequest(HandleParam &stParam);

  /**
     * 解析TUP包
     * 
     * @param buffer 
     * @param length 
     * @param tupRequest
     * @return int
     */
  int parseTupRequest(HandleParam &stParam, RequestPacket &tupRequest);

  /**
     * 解析JSON包
     * 
     * @param buffer 
     * @param length 
     * @param tupRequest
     * @return int
     */
  int parseJsonRequest(HandleParam &stParam, RequestPacket &tupRequest);

  /**
     * 解析所有的代理
     * 
     * @param mTupRequest 
     * @param map<int32_t, ServantPrx> 
     * @return int
     */
  //ServantPrx parseTupProxy(const BasePacket &tupRequest, HandleParam &stParam);

  /**
     * 异步发送
     * 
     * @param tup 
     * @param proxy 
     */
  void tupAsyncCall(RequestPacket &tup, ServantPrx &proxy, const TupCallbackPtr &cb, THashInfo::E_HASH_TYPE ht, const string &sHttpHeaderValue);

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

