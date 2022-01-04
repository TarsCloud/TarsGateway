#ifndef _TupCallback_H_
#define _TupCallback_H_

#include "servant/Application.h"
#include "util/tc_http.h"
#include "TupBase.h"
#include "Verify.h"

using namespace std;
using namespace tars;
using namespace Base;

struct TupCallbackParam
{
    int64_t                     iTime;

    /**
     * 请求是否是压缩过的
     */
    pair<string, string>        pairAcceptZip;

    /**
     * 请求是否可以加密
     */
    pair<string, string>        pairAcceptEpt;
    
    string                      sReqXua;

    string                      sReqGuid;

    string                      sReqIP;

    int                         iEptType;
    int                         iZipType;

    string                      sServantName;
    string                      sFuncName;
    tars::Int32                  iRequestId;

	string						sEncryptKey;
	size_t						iReqBufferSize;
    bool                        httpKeepAlive;
    bool                        isRestful;
};


/**
 * 异步回调对象
 */
class TupCallback : public ServantProxyCallback
{
public:

    TupCallback(const string& type, 
                const TarsCurrentPtr& current,
                const shared_ptr<TupCallbackParam> stParam, bool keepAlive) 
				:_stParam(stParam), _bKeepAlive(keepAlive), _iNewRequestId(0), _current(current)
    {
        setType(type);
    }

    /**
     * 析构
     */
    virtual ~TupCallback();

    

    void setNewRequestId(tars::Int32 id)
    {
        _iNewRequestId = id;
    }

    string getClientIp() const
    {
        return _stParam->sReqIP;
    }

    const string getServantName() const
    {
        return _stParam->sServantName;
    }
    
    void setTraceKey(const string& k)
    {
        _traceKey = k;
    }

    string getTraceKey() const
    {
        return _traceKey;
    }

    virtual int onDispatch(ReqMessagePtr msg);

protected:

    /**
     * 响应
     * 
     * @param buffer 
     */
    void doResponse_tup(const vector<char> &buffer);
    void doResponse_tars(shared_ptr<ResponsePacket> tup);
    void doResponse_json(shared_ptr<ResponsePacket> tup);
    void doResponseException(int ret, const vector<char> &buffer);

    /**
     * 相关发送应答数据处理
     */
    void handleResponse();

protected:
    /**
     * 请求相关参数
     */
    shared_ptr<TupCallbackParam>    _stParam;



    /**
     * 所有的回应包
     */
    vector<char>                _rspBuffer;
    //string                      _rspData;

    /**
     * 现在正在回包的servantname
     */
    bool                        _bKeepAlive;

    tars::Int32                  _iNewRequestId;
    tars::TarsCurrentPtr          _current;

    string                      _traceKey;
};

typedef TC_AutoPtr<TupCallback> TupCallbackPtr;

class VerifyCallback: public VerifyPrxCallback
{
public:
    VerifyCallback(ServantPrx proxy, shared_ptr<RequestPacket> request, shared_ptr<HandleParam> param, const THashInfo& hi, const string& token)
        :_proxy(proxy), _request(request), _param(param), _hashInfo(hi), _token(token) 
    {}

    virtual void callback_verify(tars::Int32 ret,  const Base::VerifyRsp& rsp);
    virtual void callback_verify_exception(tars::Int32 ret);

private:
    ServantPrx  _proxy;
    shared_ptr<RequestPacket>  _request;
    shared_ptr<HandleParam> _param;
    THashInfo   _hashInfo;
    string      _token;
};

typedef TC_AutoPtr<VerifyCallback> VerifyCallbackPtr;

/////////////////////////////////////////////////////
#endif
