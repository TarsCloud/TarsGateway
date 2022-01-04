#ifndef _ProxyImp_H_
#define _ProxyImp_H_

#include "servant/Application.h"
#include "util/tc_http.h"
#include "tupproxy/TupBase.h"
#include "tupproxy/TupCallback.h"
#include "httpproxy/HttpBase.h"
#include "proxybase/ProxyParam.h"

using namespace std;
using namespace tars;


/**
 *
 *
 */
class ProxyImp : public tars::Servant, public TupBase, public HttpBase
{
  public:
    ProxyImp();
    /**
     *
     */
    virtual ~ProxyImp();

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
     * 处理客户端的主动请求
     * @param current 
     * @param response 
     * @return int 
     */
    virtual int doRequest(tars::TarsCurrentPtr current, vector<char>& response);

protected:

    /**
     * 过滤自动监控测试请求
     * 
     */
    void filterMonitor(shared_ptr<HandleParam> stParam);

    E_PROXY_TYPE parseReqType(const string &reqUrl, const string& host);

  protected:
};


/////////////////////////////////////////////////////
#endif
