#ifndef _WupCallback_H_
#define _WupCallback_H_

#include "servant/Application.h"
#include "util/tc_http.h"
#include "WupBase.h"

using namespace std;
using namespace tars;


struct WupCallbackParam
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
class WupCallback : public ServantProxyCallback
{
public:

    WupCallback(const string& type, 
                const TarsCurrentPtr& current,
                const WupCallbackParam& stParam) 
				:_stParam(stParam), _bKeepAlive(false), _iNewRequestId(0), _current(current)
    {
        setType(type);
    }

    /**
     * 析构
     */
    virtual ~WupCallback();

    

    void setNewRequestId(tars::Int32 id)
    {
        _iNewRequestId = id;
    }

    string getClientIp() const
    {
        return _stParam.sReqIP;
    }

    virtual int onDispatch(ReqMessagePtr msg);

protected:

    /**
     * 响应
     * 
     * @param buffer 
     */
    void doResponse_wup(const vector<char> &buffer);
    void doResponse_jce(shared_ptr<ResponsePacket> wup);
    void doResponse_json(shared_ptr<ResponsePacket> wup);
    void doResponseException(int ret, const vector<char> &buffer);

    /**
     * 相关发送应答数据处理
     */
    void handleResponse();

protected:
    /**
     * 请求相关参数
     */
    WupCallbackParam            _stParam;



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

};

typedef TC_AutoPtr<WupCallback> WupCallbackPtr;


/////////////////////////////////////////////////////
#endif
