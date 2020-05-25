#include "HttpRouter.h"
#include "StationManager.h"

#define str2_cmp(m, c0, c1)                                 \
    m[0] == c0 && m[1] == c1

#define str3_cmp(m, c0, c1, c2)                         \
    m[0] == c0 && m[1] == c1 && m[2] == c2

#define str4_cmp(m, c0, c1, c2, c3)                         \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3

static string id2str(int i)
{
    char ss[11] = {0};
    sprintf(ss,"%05d",i);
    return string(ss);
}

static bool regInit(const string& p, bool isCase, regex_t& reg)
{
    int ret = regcomp(&reg, p.c_str(), (isCase ? 0:REG_ICASE));
    if (ret != 0)
    {
        char errmsg[128] = {0};
        regerror(ret, &reg, errmsg, sizeof(errmsg));
        TLOGERROR(p << "|iscase:" << isCase << ", regcomp error:" << ret << "|" << errmsg << endl);
        return false;
    }
    return true;
}

static bool regMatch(const string& s, const regex_t& reg)
{
    regmatch_t matchData[10];

    //return (regexec(&reg, s.c_str(), 10, matchData, 0) == 0);
    int ret = regexec(&reg, s.c_str(), 10, matchData, 0);

    if( REG_NOMATCH ==  ret )
    {
        return false;
    }
    else if (ret != 0)
    {
        return false;
    }
    else
    {                                                                                                                              
        return true;
    }
    return false;
}

string RouterPath::getOrderKey() const
{
    return id2str(_orderID) + "-" + _path;
}

bool RouterPath::operator<(const RouterPath &rp) const
{
    string k1 = getOrderKey();
    string k2 = rp.getOrderKey();

    if (k1.length() > k2.length())
        return true;
    else if (k1.length() < k2.length())
        return false;
    else
        return (k1 < k2);
}

RouterPath::E_PATH_TYPE RouterPath::initPath(int id, const string& path, const string& pp, const string& stationId)
{
    _orderID = id;

    size_t pos = 0;
    if ("/" == path)
    {
        // / 任何请求都会匹配
        _type = EPT_DEFAULT;
    }
    else if (path.length() > 2 && str2_cmp(path, '=', ' '))
    {
        // ^~ uri以某个常规字符串开头： ^~ /static/
        _type = EPT_FULL;
        pos = 2;
    }
    else if (path.length() > 2 && str2_cmp(path, '~', ' '))
    {
        // ~ 正则匹配(区分大小写)：  ~ \.(gif|jpg|png|js|css)$
        _type = EPT_REGSUCC;
        _isCase = true;
        pos = 2;
    }
    else if (path.length() > 3 && str3_cmp(path, '~', '*', ' '))
    {   
        // ~* 正则匹配(不区分大小写)：~* \.png$
        _type = EPT_REGSUCC;
        _isCase = false;
        pos = 3;
    }
    else if (path.length() > 3 && str3_cmp(path, '!', '~', ' '))
    {
        // !~ 不匹配正则(区分大小写)：  !~ \.xhtml$
        _type = EPT_REGFAIL;
        _isCase = true;
        pos = 3;
    }
    else if (path.length() > 4 && str4_cmp(path, '!', '~', '*', ' '))
    {
        // !~ 不匹配正则(不区分大小写)：  !~* \.xhtml$
        _type = EPT_REGFAIL;
        _isCase = false;
        pos = 4;
    }
    else if (path.length() > 3 && str3_cmp(path, '^', '~', ' '))
    {
        // ^~ uri以某个常规字符串开头： ^~ /static/
        _type = EPT_STARTWITH;
        pos = 3;
    }
    else if (path.length() > 0 && path[0] == '/')
    {
        // 直接 以 / 开头， 路径通配
        _type = EPT_COMMPATH;
    }
    else
    {
        TLOGERROR("error path:" << path << endl);
        return EPT_NULL;
    }

    _path = TC_Common::trim(path.substr(pos));

    if ((_type == EPT_REGSUCC || _type == EPT_REGFAIL) && (!regInit(_path, _isCase, _reg)) )
    {
        // 初始化正则失败
        TLOGERROR(path << ", regInit fail!" << endl);
        return EPT_NULL;
    }

    TLOGDEBUG("parse path succ:" << path << "|" << _type << "|" << _path << endl);

    // 初始化 proxy_pass
    if (pp.length() < 8 || strncmp(pp.c_str(), "http://", 7) != 0)
    {
        TLOGERROR("error proxy_pass:" << pp << ", " << path << endl);
        return EPT_NULL;
    }
    string::size_type x = pp.find("/", 8);
    if (x != string::npos)
    {
        _proxyHost = pp.substr(7, x - 7);
        _proxyPath = pp.substr(x);
    }
    else
    {
        _proxyHost = pp.substr(7);
        _proxyPath = "";
    }
    _stationId = stationId;

    TLOGDEBUG("parse proxy_pass succ:" << pp << "|" << _proxyHost << "|" << _proxyPath << endl);

    if (_proxyHost.length() > 3 && strncmp(_proxyHost.c_str() + (_proxyHost.length() -3), "Obj", 3) == 0)
    {
        // tafobj 类型，添加到定时更新地址列表
        STATIONMNG->addObjUpstream(_proxyHost);
    }

    FDLOG("load") << path << "|" << pp << "|result:" << _type << "|" << _path << "|" << _proxyHost << "|" << _proxyPath << endl;
    
    return _type;
}

bool RouterPath::match(const string& p, RouterResult& result)
{
    switch (_type)
    {
        case EPT_FULL:
            if (p != _path)
            {
                TLOGERROR("match fail, path:" << p << ", config path:" << _path << endl);
                return false;
            }
            break;
        case EPT_STARTWITH:
            if (p.length() < _path.length() || strncmp(p.c_str(), _path.c_str(), _path.length()) != 0)
            {
                return false;
            }
            break;
        case EPT_REGSUCC:
            if (!regMatch(p, _reg))
            {
                return false;
            }
            else
            {
                TLOGDEBUG("match path succ, reg:" << _path << ", path:" << p << endl);
            }

            break;
        case EPT_REGFAIL:
            if (regMatch(p, _reg))
            {
                return false;
            }
            break;
        case EPT_COMMPATH:
            if (p.length() < _path.length() || strncmp(p.c_str(), _path.c_str(), _path.length()) != 0)
            {
                return false;
            }
            break;
        case EPT_DEFAULT:
            break;
        default:
            return false;
    }

    return genResult(p, result);
}

bool RouterPath::genResult(const string &p, RouterResult& result)
{
    // 这里先不考虑 rewrite
    size_t pos = 0;
    if (_type == EPT_REGSUCC || _type == EPT_REGFAIL || _proxyPath.empty())
    {
        result.path = p;
    }
    else if (p.length() >= _path.length())
    {
        result.path = _proxyPath + p.substr(_path.length());
        pos = _path.length();
    }
    else
    {
        // 其他异常情况， 还是直接透传
        result.path = p;
    }

    if (p.length() > pos + 1)
    {
        result.funcPath = p.substr(0, p.find("/", pos + 1));
    }
    else
    {
        result.funcPath = p;
    }

    result.upstream = _proxyHost;
    result.stationId = _stationId;

    TLOGDEBUG(p << " match path succ. Type:" << _type << ", result path:" << result.path << ", upstream:" << result.upstream << ", station:" << result.stationId << ", funcPath:" << result.funcPath << endl);

    return true;
}

bool RouterHost::operator<(const RouterHost &rh)
{
    if (getOrderKey().length() > rh.getOrderKey().length())
        return true;
    else if (getOrderKey().length() < rh.getOrderKey().length())
        return false;
    else
        return (getOrderKey() < rh.getOrderKey());    
}

RouterHost::E_HOST_TYPE RouterHost::getHostType(const string& serverName)
{
    RouterHost::E_HOST_TYPE type = EHT_NULL;
    if (serverName.empty())
    {
        type = EHT_DEFAULT;
    }
    else if (serverName.length() > 1 && serverName.front() == '*')
    {
        type = EHT_PREWCMATCH;
    }
    else if (serverName.length() > 1 && serverName.back() == '*')
    {
        type = EHT_SUFWCMATCH;
    }
    else if (serverName.length() > 2 && serverName.front() == '~')
    {
        type = EHT_REGMATCH;
    }
    else 
    {
        type = EHT_FULL;
    }
    return type;
}

bool RouterHost::initHost(const string& serverName)
{
    _type = getHostType(serverName);
    switch (_type)
    {
    case EHT_DEFAULT:
        _host = serverName;
        break;
    case EHT_PREWCMATCH:
        _host = serverName.substr(1);
        break;
    case EHT_SUFWCMATCH:
        _host = serverName.substr(0, serverName.length() - 1);
        break;
    case EHT_REGMATCH:
        _host = serverName.substr(1);
        if (!regInit(_host, regInit, _reg))
        {
            TLOGERROR("init regex fail:" << serverName << "|" << _host << endl);
            return false;
        }
        break;
    case EHT_FULL:
        _host = serverName;
        break;
    default:
        return false;
    }
    
    return true;
}

bool RouterHost::addPath(int id, const string& location, const string& proxyPass, const string& stationId)
{
    RouterPathPtr rp(new RouterPath());
    RouterPath::E_PATH_TYPE type = rp->initPath(id, location, proxyPass, stationId);
    switch (type)
    {
    case RouterPath::EPT_FULL:
        _fullPath[rp->getPath()] = rp;
        break;
    case RouterPath::EPT_STARTWITH:
        _startWithPath.insert(rp);
        break;
    case RouterPath::EPT_REGSUCC:
        _regSuccPath.insert(rp);
        break;
    case RouterPath::EPT_REGFAIL:
        _regFailPath.insert(rp);
        break;
    case RouterPath::EPT_COMMPATH:
        _commPath.insert(rp);
        break;
    case RouterPath::EPT_DEFAULT:
        _defaultPath = rp;
        break;
    default:
        break;
    }
    return (type != RouterPath::EPT_NULL);
}

bool RouterHost::matchHost(const string &h)
{
    if (_type == EHT_FULL)
    {
        return h == _host;
    }
    else if (_type == EHT_PREWCMATCH)
    {
        return (h.length() >= _host.length() && strncmp(h.c_str() + (h.length() - _host.length()), _host.c_str(), _host.length()) == 0);
    }
    else if (_type == EHT_SUFWCMATCH)
    {
        return (h.length() >= _host.length() && strncmp(h.c_str(), _host.c_str(), _host.length()) == 0);
    }
    else if (_type == EHT_REGMATCH)
    {
        return regMatch(h, _reg);
    }
    else if (_type == EHT_DEFAULT)
    {
        return true;
    }
    return false;
}

bool RouterHost::matchPath(const string &p, RouterResult& result)
{
    auto it = _fullPath.find(p);
    if (it != _fullPath.end())
    {
        return it->second->match(p, result);
    }

    bool ret = false;
    for (auto t = _startWithPath.begin(); t != _startWithPath.end(); ++t)
    {
        if ((*t)->match(p, result))
        {
            return true;
        }
    }
    for (auto t = _regSuccPath.begin(); t != _regSuccPath.end(); ++t)
    {
        if ((*t)->match(p, result))
        {
            return true;
        }
    }
    for (auto t = _regFailPath.begin(); t != _regFailPath.end(); ++t)
    {
        if ((*t)->match(p, result))
        {
            return true;
        }
    }
    for (auto t = _commPath.begin(); t != _commPath.end(); ++t)
    {
        if ((*t)->match(p, result))
        {
            return true;
        }
    }
    if (_defaultPath)
    {
        return _defaultPath->match(p, result);
    }

    return false;
}

bool HttpRouter::addRouter(int id, const string& serverName, const string& location, const string& proxyPass, const string& stationId)
{
    RouterHost::E_HOST_TYPE type = RouterHost::getHostType(serverName);
    bool ret = false;
    TC_ThreadWLock w(_rwLock);
    if (RouterHost::EHT_DEFAULT == type)
    {
        if (!_defaultHost)
        {
            _defaultHost = make_shared<RouterHost>(serverName);
        }
        ret = _defaultHost->addPath(id, location, proxyPass, stationId);
    }
    else if (RouterHost::EHT_FULL == type)
    {
        if (_fullHost.find(serverName) == _fullHost.end())
        {
            _fullHost[serverName] = make_shared<RouterHost>(serverName);
        }
        ret = _fullHost[serverName]->addPath(id, location, proxyPass, stationId);
    }
    else if (RouterHost::EHT_PREWCMATCH == type)
    {
        if (_preWCMatchHost.find(serverName) == _preWCMatchHost.end())
        {
            _preWCMatchHost[serverName] = make_shared<RouterHost>(serverName);
        }
        ret = _preWCMatchHost[serverName]->addPath(id, location, proxyPass, stationId);
    }
    else if (RouterHost::EHT_SUFWCMATCH == type)
    {
        if (_sufWCMatchHost.find(serverName) == _sufWCMatchHost.end())
        {
            _sufWCMatchHost[serverName] = make_shared<RouterHost>(serverName);
        }
        ret = _sufWCMatchHost[serverName]->addPath(id, location, proxyPass, stationId);
    }
    else if (RouterHost::EHT_REGMATCH == type)
    {
        if (_regHost.find(serverName) == _regHost.end())
        {
            _regHost[serverName] = make_shared<RouterHost>();
            if (!_regHost[serverName]->initHost(serverName))
            {
                TLOGERROR("init RouterHost fail:" << serverName << endl);
                _regHost.erase(serverName);
                return false;
            }
        }
        ret = _regHost[serverName]->addPath(id, location, proxyPass, stationId);
    }

    TLOGDEBUG(id << "|" << serverName << "|" << location << "|" << proxyPass << "|" << ret << endl);
    return true;
}

bool RouterAgent::reload(const vector<RouterParam> &param)
{
    shared_ptr<HttpRouter> router = make_shared<HttpRouter>();

    for (size_t i = 0; i < param.size(); i++)
    {
        if(!router->addRouter(param[i].id, param[i].serverName, param[i].location, param[i].proxyPass, param[i].stationId))
        {
            TLOGERROR("reload fail!" << param[i].id << "|" << param[i].serverName << "|" << param[i].location << "|" << param[i].stationId << endl);
            return false;
        }
    }

    _router.swap(router);
    //_router = router;
    TLOGDEBUG("reload router succ, total router:" << param.size() << endl);
    return true;
}

bool HttpRouter::parse(const string& host, const string& path, RouterResult& result)
{
    TC_ThreadRLock r(_rwLock);
    bool ret = false;
    // 1、全匹配
    TLOGDEBUG(host << "|" << path << endl);
    auto it = _fullHost.find(host);
    if (it != _fullHost.end())
    {
        TLOGDEBUG(host << " math full host, " << it->second->getOrderKey() << endl);
        return it->second->matchPath(path, result);
    }
    else
    {
        //2、前通配符匹配
        for (LenMapIterator t = _preWCMatchHost.begin(); t != _preWCMatchHost.end(); ++t)
        {
            // host 匹配到了， 就不再匹配其他host了
            if (t->second->matchHost(host))
            {
                TLOGDEBUG(host << " math _preWCMatchHost, " << t->second->getOrderKey() << endl);
                return t->second->matchPath(path, result);
            }
        }
        //3、后通配符匹配
        for (LenMapIterator t = _sufWCMatchHost.begin(); t != _sufWCMatchHost.end(); ++t)
        {
            if (t->second->matchHost(host))
            {
                TLOGDEBUG(host << " math _sufWCMatchHost, " << t->second->getOrderKey() << endl);
                return t->second->matchPath(path, result);
            }
        }
        //4、正则匹配
        for (LenMapIterator t = _regHost.begin(); t != _regHost.end(); ++t)
        {
            if (t->second->matchHost(host))
            {
                TLOGDEBUG(host << " math regHost, " << t->second->getOrderKey() << endl);
                return t->second->matchPath(path, result);
            }
        }
        //5、默认
        if (_defaultHost)
        {
            TLOGDEBUG(host << " math default host, " << _defaultHost->getOrderKey() << endl);
            return _defaultHost->matchPath(path, result);
        }
    }

    return false;
}