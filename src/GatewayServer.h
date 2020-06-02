#ifndef _GatewayServer_H_
#define _GatewayServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class GatewayServer : public Application
{
public:
    /**
     *
     **/
    virtual ~GatewayServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();

    const string& getLocalServerName() const
    {
        return _localServerName;
    }

    size_t getRspSizeLimit() const 
    {
        return _rspSizeLimit;
    }

    // const string& getTupHost() const
    // {
    //     return _tupHost;
    // }
    const string& getTupPath() const
    {
        return _tupPath;
    }
    const string& getJsonPath() const
    {
        return _jsonPath;
    }
    const string& getJsonPathEx() const
    {
        return _jsonPathEx;
    }
    const string& getMonitorUrl() const 
    {
        return _monitorUrl;
    }
    bool isInactiveCode(int ret)
    {
        return (_inactiveRetCode.find(ret) != _inactiveRetCode.end());
    }
    bool isTimeoutCode(int ret)
    {
        return (_timeoutRetCode.find(ret) != _timeoutRetCode.end());
    }

    void setConfFile(const string& f)
    {
        _tupProxyConf = f;
    }
    const string& getConfFile() const
    {
        return _tupProxyConf;
    }

    bool isTupHost(const string &h) const;

  protected:
    /**
     * 加载配置
     * @param command
     * @param params
     * @param result
     * 
     * @return bool
     */
    bool loadProxy(const string& command, const string& params, string& result);
    bool loadHttp(const string& command, const string& params, string& result);
    bool loadComm(const string &command, const string &params, string &result);

  private:
    string _localServerName;
    size_t _rspSizeLimit;
    //string _tupHost;
    set<string> _tupFullHost;
    vector<string> _tupPreHost;
    string _tupPath;
    string _jsonPath;
    string _jsonPathEx;
    string _monitorUrl;
    set<int> _inactiveRetCode;
    set<int> _timeoutRetCode;
    string _tupProxyConf;
};

extern GatewayServer g_app;

////////////////////////////////////////////
#endif
