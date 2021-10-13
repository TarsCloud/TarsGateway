#include "HttpCallBack.h"
#include "proxybase/ProxyUtils.h"
#include "GatewayServer.h"

void AsyncHttpCallback::onSucc(TC_HttpResponse &stHttpResponse)
{
    TLOG_DEBUG("onSucc, content length:" << stHttpResponse.getContent().length() << endl);
    //TLOG_DEBUG(stHttpResponse.genHeader() << endl);
    _aLog->status = stHttpResponse.getStatus();
    _aLog->rspSize = stHttpResponse.getContent().length();
    _aLog->iRet = 0;

    // 修复http头
    //stHttpResponse.setContentLength(stHttpResponse.getContent().length());

    string rspBuff = stHttpResponse.encode();

    _current->sendResponse(rspBuff.c_str(), rspBuff.length());
    if (_addrPrx)
    {
        _addrPrx->doFinish(false); 
    }
    
    if (!_keepalive)
    {
        _current->close();
    }
}

bool AsyncHttpCallback::onContinue(TC_HttpResponse &stHttpResponse)
{
    return true;
}

void AsyncHttpCallback::onFailed(FAILED_CODE ret, const string &info)
{
    TLOG_ERROR("onFailed:" << ret << ", " << info << "|" << _url << "|" << _addrPrx->getAddr() << endl);
//    ERR_DAY_LOG << _addrPrx->getAddr() << _url << "|" << ret << "|" << info << endl;
    _aLog->status = 502;
    _aLog->errorMsg = "http fail:" + TC_Common::tostr(ret);
    
    if (_addrPrx && _addrPrx->fusingOnOff())
    {
        if (g_app.isTimeoutCode((int)ret))
        {
            // 网络超时: Failed_Timeout || Failed_Net
            _addrPrx->doFinish(true);
            _aLog->status = 504;
            _aLog->iRet = -7;
        }
        else if (g_app.isInactiveCode((int)ret))
        {
            // 连接出错， 暂时屏蔽该节点，并加入自动检测: Failed_Connect || Failed_ConnectTimeout
            if (_addrPrx)
            {
                _addrPrx->setActive(false);
                AddrCheckThread::getInstance()->addAddr(_addrPrx);
            } 
        }
        else
        {
            // 暂时不处理
            _addrPrx->doFinish(false); 
        }
    }

    ProxyUtils::doErrorRsp(_aLog->status, _current, _keepalive);
}

void AsyncHttpCallback::onClose()
{
    TLOG_DEBUG("onClose" << endl);
}