#include "TupProxyManager.h"
#include "GatewayServer.h"
#include "servant/Application.h"
#include "util/tc_config.h"
#include "util/tc_mysql.h"
#include "Verify.h"

TupProxyManager::TupProxyManager()
{
    _prot_tup.responseFunc = ProxyProtocol::totalResponse;
    //_terminate              = false;
    _autoProxy = false;
}

// void TupProxyManager::terminate()
// {
//     _terminate = true;

//     TC_ThreadLock::Lock lock(*this);
//     notifyAll();
// }

string TupProxyManager::loadProxy(const TC_Config &conf)
{
    try
    {
        _autoProxy = (bool)TC_Common::strto<int>(conf.get("/main<auto_proxy>", "0"));

        map<string, string> nameMap;
        map<string, string> funcMap;
        map<string, string> specMap;

        TC_LockT<TC_ThreadMutex> lock(_mutex);

        if (conf.getDomainMap("/main/proxy", nameMap))
        {
            _nameMap.swap(nameMap);

            TLOG_DEBUG("nameMap:" << TC_Common::tostr(_nameMap) << endl);
        }

        vector<string> nameVector;

        if (conf.getDomainVector("/main/proxy", nameVector))
        {
            for (size_t i = 0; i < nameVector.size(); i++)
            {
                map<string, string> m;
                if (conf.getDomainMap("/main/proxy/" + nameVector[i], m))
                {
                    map<string, string>::iterator it = m.begin();
                    while (it != m.end())
                    {
                        _nameMap[nameVector[i] + "." + it->first] = it->second;
                        ++it;
                    }
                }
            }
        }

        for (auto it = _nameMap.begin(); it != _nameMap.end(); ++it)
        {
            string k = it->first;
            vector<string> vs = TC_Common::sepstr<string>(it->first, ":");
            if (vs.size() == 2)
            {
                vector<string> funcList = TC_Common::sepstr<string>(vs[1], "|");
                for (size_t i = 0; i < funcList.size(); i++)
                {
                    k = vs[0] + ":" + funcList[i];
                    _nameMap[k] = it->second;
                    updateHashInfo(k, it->second);
                }
            }
            else
            {
                updateHashInfo(k, it->second);
            }
        }

        TLOG_DEBUG(" allproxy:" << TC_Common::tostr(_nameMap) << endl);

        initVerifyInfo(conf);

        _lastUpdateTime = 0;
        _lastUpdateTotalNum = 0;

        _httpHeaderForProxy.clear();

        map<string, string> httpHeader;
        if (conf.getDomainMap("/main/env_httpheader", httpHeader))
        {
            _httpHeader.clear();

            map<string, string>::iterator it = httpHeader.begin();
            while (it != httpHeader.end())
            {
                _httpHeader[TC_Common::upper(it->first)] = it->second;

                vector<string> v = TC_Common::sepstr<string>(TC_Common::upper(it->first), ": ", false);

                if (v.size() > 0)
                {
                    _httpHeaderForProxy.insert(v[0]);
                }
                else
                {
                    TLOG_ERROR("[TupProxyManager::loadHttpHeader] httpheader error:" << it->first << endl);
                }

                ++it;
            }

            TLOG_DEBUG("[TupProxyManager::loadHttpHeader] " << TC_Common::tostr(_httpHeader) << endl);
        }

        return "load ok";
    }
    catch (exception &ex)
    {
        TLOG_ERROR("[TupProxyManager::loadProxy] error:" << ex.what() << endl);
        return string("loadProxy config error:") + ex.what();
    }

    return "loadProxy error.";
}

void TupProxyManager::initVerifyInfo(const TC_Config& conf)
{
    map<string, VerifyInfo>     proxyVerify;
    set<string>                 noVerify;

    vector<string> authVector;
    if(conf.getDomainVector("/main/auth", authVector))
    {
        for(size_t i = 0; i < authVector.size(); i++)
        {
            VerifyInfo info;
            string authObj = conf.get("/main/auth/" + authVector[i] + "<verify>");
            if (authObj.empty())
            {
                TLOG_ERROR("auth info init fail:" << "/main/auth/" << authVector[i] << "<verify> is empty!" << endl);
                continue;
            }
            info.prx = Application::getCommunicator()->stringToProxy<Base::VerifyPrx>(authObj + "#99999");
            info.tokenHeader = conf.get("/main/auth/" + authVector[i] + "<auth_http_header>");
            if (info.tokenHeader.empty())
            {
                TLOG_ERROR("auth info init fail:" << "/main/auth/" << authVector[i] << "<auth_http_header> is empty!" << endl);
                continue;
            }
            info.verifyBody = (TC_Common::lower(conf.get("/main/auth/" + authVector[i] + "<verify_body>", "false")) == "true");
            info.verifyHeaders = TC_Common::sepstr<string>(conf.get("/main/auth/" + authVector[i] + "<verify_headers>"), "|");
            vector<string> lines = conf.getDomainLine("/main/auth/" + authVector[i] + "/auth_list");
            for (auto it = lines.begin(); it != lines.end(); it++)
            {
                vector<string> vs = TC_Common::sepstr<string>(*it, ":");
                if (vs.size() == 2)
                {
                    vector<string> funcList = TC_Common::sepstr<string>(vs[1], "|");
                    for (size_t i = 0; i < funcList.size(); i++)
                    {
                        proxyVerify[vs[0] + ":" + funcList[i]] = info;
                        TLOG_DEBUG("add verify|" << vs[0] + ":" + funcList[i] << "|" << authObj << endl);
                    }               
                }
                else
                {
                    proxyVerify[*it] = info;
                    TLOG_DEBUG("add verify|" << *it << "|" << authObj << endl);
                }      
            }
            
            lines = conf.getDomainLine("/main/auth/" + authVector[i] + "/auth_list/exclude");
            for (auto it = lines.begin(); it != lines.end(); it++)
            {
                vector<string> vs = TC_Common::sepstr<string>(*it, ":");
                if (vs.size() == 2)
                {
                    vector<string> funcList = TC_Common::sepstr<string>(vs[1], "|");
                    for (size_t i = 0; i < funcList.size(); i++)
                    {
                        noVerify.insert(vs[0] + ":" + funcList[i]);
                        TLOG_DEBUG("exclude verify|" << vs[0] + ":" + funcList[i] << endl);
                    }               
                }
                else
                {
                    noVerify.insert(*it);
                    TLOG_DEBUG("exclude verify|" << *it << endl);
                }      
            }
            
        }
    }

    _proxyVerify.swap(proxyVerify);
    _noVerify.swap(noVerify);
    TLOG_DEBUG("init verify ok" << endl);
}

void TupProxyManager::updateHashInfo(const string &servantName, const string &obj)
{
    // 这里不再加锁，所以在调这个函数的地方注意要 已经加锁
    auto itp = _proxyMap.find(servantName);
    if (itp != _proxyMap.end())
    {
        parseHashInfo(obj, itp->second.second);
        TLOG_DEBUG(servantName << "|" << obj << "|hash:" << itp->second.second.type << "-" << itp->second.second.httpHeadKey << endl);
    }
}

string TupProxyManager::parseHashInfo(const string &objInfo, THashInfo &hi)
{
    string realObj;
    vector<string> vsServant = TC_Common::sepstr<string>(objInfo, "|");
    if (vsServant.size() > 1)
    {
        int type = TC_Common::strto<int>(vsServant[1]);
        if (0 == type)
        {
            hi.type = THashInfo::EHT_ROBINROUND;
        }
        else if (1 == type)
        {
            hi.type = THashInfo::EHT_REQUESTID;
        }
        else if (2 == type && vsServant.size() == 3)
        {
            hi.type = THashInfo::EHT_HTTPHEAD;
            hi.httpHeadKey = TC_Common::trim(vsServant[2]);
        }
        else if (3 == type)
        {
            hi.type = THashInfo::EHT_CLIENTIP;
        }
        else
        {
            hi.type = THashInfo::EHT_DEFAULT;
        }
        realObj = vsServant[0];
    }
    else if (vsServant.size() > 0)
    {
        hi.type = THashInfo::EHT_DEFAULT;
        realObj = vsServant[0];
    }
    return TC_Common::trim(realObj);
}

ServantPrx TupProxyManager::getProxy(const string& sServantName, const string& sFuncName, const TC_HttpRequest &httpRequest, ProxyExInfo& pei)
{
    ServantPrx prx = getProxy(sServantName, sFuncName, httpRequest, pei.hashInfo);
    if (prx)
    {
        TC_LockT<TC_ThreadMutex> lock(_mutex);
        string servant = string(prx->tars_name().substr(0, prx->tars_name().find("@")));
        string serverFunc = servant + ":" + sFuncName;
        if (_noVerify.find(servant) != _noVerify.end() || _noVerify.find(serverFunc) != _noVerify.end())
        {
            return prx;
        }
        if (_proxyVerify.find(servant) != _proxyVerify.end())
        {
            pei.verifyInfo = _proxyVerify[servant];
            return prx;
        }
        if (_proxyVerify.find(serverFunc) != _proxyVerify.end())
        {
            pei.verifyInfo = _proxyVerify[servant];
            return prx;
        }
        string app = servant.substr(0, servant.find(".")) + ".*";
        if (_proxyVerify.find(app) != _proxyVerify.end())
        {
            pei.verifyInfo = _proxyVerify[app];
            return prx;
        }
    }
    return prx;
}

//如果没有配置servantname,则根据配置<funcname></funcname>来进行赋值。
ServantPrx TupProxyManager::getProxy(const string &sServantName, const string &sFuncName, const TC_HttpRequest &httpRequest, THashInfo &hi)
{
    string name = sServantName;

    TC_LockT<TC_ThreadMutex> lock(_mutex);

    //先查找头部有没有特殊转发的地址
    set<string>::iterator it = _httpHeaderForProxy.begin();
    while (it != _httpHeaderForProxy.end())
    {
        string value = httpRequest.getHeader(*it);

        if (!value.empty() && value != "00000000000000000000000000000000" && value != "0000000000000000" && value != "00" && value != "0")
        {
            map<string, string>::iterator it1 = _httpHeader.find((*it) + ":" + TC_Common::upper(value));
            if (it1 != _httpHeader.end())
            {
                name = it1->second + "." + sServantName;
                break;
            }
        }
        ++it;
    }

    //查找内部的实际Servant名称
    ServantPrx proxy;
    string realServantName;

    {
        string nameFunc = name + ":" + sFuncName;
        auto itFun = _proxyMap.find(nameFunc);
        if (itFun != _proxyMap.end())
        {
            TLOG_DEBUG("rpc-call---------------> " << itFun->second.first->tars_name() << " :: " << nameFunc << endl);
            hi = itFun->second.second;
            return itFun->second.first;
        }

        auto itName = _nameMap.find(nameFunc);
        if (itName != _nameMap.end()) //在配置里面找到了name，则找出对应的realServantname
        {
            realServantName = itName->second;
            name = nameFunc;
            TLOG_DEBUG("nameFunc|name:" << name << ", sServantName:" << sServantName << ", sFuncName:" << sFuncName << ", " << realServantName << endl);
        }
        else
        {
            auto it2 = _proxyMap.find(name);
            if (it2 != _proxyMap.end())
            {
                TLOG_DEBUG("rpc-call---------------> " << it2->second.first->tars_name() << " :: " << name << endl);
                hi = it2->second.second;
                return it2->second.first;
            }
        }
    }

    //绝大部分情况下, 运行到上面就直接返回了
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (realServantName.empty())
    {
        map<string, string>::iterator it1 = _nameMap.find(name);
        if (it1 != _nameMap.end()) //在配置里面找到了name，则找出对应的realServantname
        {
            realServantName = it1->second;
            TLOG_DEBUG("name:" << name << ", sServantName:" << sServantName << ", sFuncName:" << sFuncName << ", " << realServantName << endl);
        }
    }

    //没有实际的servant名称,客户端打包进去的servantname在配置文件找不到对应的真实servantname
    if (realServantName.empty())
    {
        if (_autoProxy)
        {
            vector<string> serverObj;
            string::size_type pos = sServantName.find("@");
            if (pos != string::npos)
            {
                serverObj = TC_Common::sepstr<string>(sServantName.substr(0, pos), ".");
            }
            else
            {
                serverObj = TC_Common::sepstr<string>(sServantName, ".");
            }

            if (serverObj.size() >= 3)
            {
                realServantName = sServantName;
            }
        }

        if (realServantName.empty())
        {
            TLOG_ERROR(sServantName << ", " << name << ", no proxy" << endl);
            return NULL;
        }
    }

    realServantName = parseHashInfo(realServantName, hi);

    proxy = Application::getCommunicator()->stringToProxy<ServantPrx>(realServantName);

    TLOG_DEBUG("add_new_proxy, " << sServantName << ":" << sFuncName << "--->" << realServantName << ", hashtype:" << hi.type << "-" << hi.httpHeadKey << endl);

    if (_realnameSet.find(realServantName) == _realnameSet.end())
    {
        proxy->tars_set_protocol(_prot_tup);
        _realnameSet.insert(realServantName); //设置过协议解析的proxy,把对应的名字添加到set去，下次就不再设置
    }
    else
    {
        TLOG_ERROR(realServantName << " has already set the protocol , no need reset." << endl);
    }

    if (!name.empty())
    {
        //name不为空才加到cache里面
        _proxyMap[name] = make_pair(proxy, hi);
    }

    return proxy;
}

/*
void TupProxyManager::run()
{
    _lastUpdateTime = 0;
    _lastUpdateTotalNum= 0;

    while(!_terminate)
    {
//        TLOG_DEBUG("[TupProxyManager] run load proxy, time=" << _lastUpdateTime << ", _lastUpdateTotalNum:" << _lastUpdateTotalNum << endl);

        try
        {
            TC_Config conf;
            //conf.parseFile(ServerConfig::BasePath + "TupProxyServer.conf");
            conf.parseFile(g_app.getConfFile());
            map<string, string> db;

            if(conf.getDomainMap("/main/db", db))
            {
                TC_DBConf tcDBConf;
                tcDBConf.loadFromMap(db);

                TC_Mysql mysql;

                mysql.init(tcDBConf);

                string sql = "select `guid`, `env`, `name`, UNIX_TIMESTAMP(updatetime) AS updatetime "  
                             "from t_user_guid_info order by updatetime desc";

                TC_Mysql::MysqlData data = mysql.queryRecord(sql);
                int iCurTotalNum= data.size();

                if(data.size() > 0)
                {
                    time_t t = TC_Common::strto<time_t>(data[0]["updatetime"]);

                    //有更新了
                    if (_lastUpdateTime != t || _lastUpdateTotalNum != iCurTotalNum)
                    {
                        TLOG_DEBUG("[TupProxyManager::run] new update test guid info, size:" << data.size() << endl);

                        _lastUpdateTime = t;
                        _lastUpdateTotalNum = iCurTotalNum;

                        map<string, string> httpHeader;

                        for(size_t i = 0; i < data.size(); i++)
                        {
                            httpHeader["Q-GUID:" + TC_Common::upper(TC_Common::trim(data[i]["guid"]))] = data[i]["env"];

                            TLOG_DEBUG(data[i]["guid"] << ":" << data[i]["env"] << ":" << data[i]["name"] << endl);
                        }

                        TC_LockT<TC_ThreadMutex> lock(_mutex);
//                        TC_ThreadLock::Lock lock(*this);

                        _httpHeader.clear();

                        _httpHeaderForProxy.insert("Q-GUID");

                        if ( httpHeader.size() > 0 )
                        {
                            _httpHeader.insert(httpHeader.begin(), httpHeader.end());
                        }
                    }
                    else
                    {
                        TLOG_DEBUG("[TupProxyManager::run] t = _lastUpdateTime, no update." << endl);
                    }
                }
            }
        }
        catch(exception &ex)
        {
            TLOG_ERROR("exception:" << ex.what() << endl);
        }
        catch(...)
        {
            TLOG_ERROR("exception unknown error." << endl);
        }

        TC_ThreadLock::Lock lock(*this);
        timedWait(1000 * 600);
    }
}
*/
