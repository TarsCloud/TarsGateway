#ifndef _GetwayServer_H_
#define _GetwayServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class GetwayServer : public Application
{
public:
    /**
     *
     **/
    virtual ~GetwayServer() {};

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

    // const string& getWupHost() const
    // {
    //     return _wupHost;
    // }
    const string& getWupPath() const
    {
        return _wupPath;
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
        _wupProxyConf = f;
    }
    const string& getConfFile() const
    {
        return _wupProxyConf;
    }

    bool isWupHost(const string &h) const;

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
    //string _wupHost;
    set<string> _wupFullHost;
    vector<string> _wupPreHost;
    string _wupPath;
    string _jsonPath;
    string _jsonPathEx;
    string _monitorUrl;
    set<int> _inactiveRetCode;
    set<int> _timeoutRetCode;
    string _wupProxyConf;
};

extern GetwayServer g_app;

////////////////////////////////////////////
#endif
