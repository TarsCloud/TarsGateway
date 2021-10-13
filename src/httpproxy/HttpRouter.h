#ifndef _HTTP_ROUTER_H_
#define _HTTP_ROUTER_H_

#include <map>
#include <string>
#include <regex.h>
#include <set>
#include "util/tc_common.h"
#include "servant/TarsLogger.h"
#include "util/tc_thread_rwlock.h"
#include "util/tc_singleton.h"
#include "util/tc_monitor.h"

using namespace tars;
using namespace std;

struct RouterResult
{
    string upstream;
    string path;
    string stationId;
    string funcPath;
};

struct RouterParam
{
    int id;
    string serverName;
    string location;
    string proxyPass;
    string stationId;
};

class RouterPath
{
public:
	enum E_PATH_TYPE
	{
        EPT_NULL    = 0,
		EPT_FULL    = 1,
		EPT_STARTWITH = 2,
		EPT_REGSUCC = 3,
		EPT_REGFAIL = 4,
        EPT_COMMPATH= 5,
		EPT_DEFAULT = 99,
	};

    RouterPath(): _type(EPT_NULL)  {}

    ~RouterPath()
    {
        if (_type == EPT_REGSUCC || _type == EPT_REGFAIL)
        {
            regfree(&_reg);
        }
    }

    E_PATH_TYPE initPath(int id, const string& path, const string& pp, const string& stationId);
    bool match(const string& p, RouterResult& result);

    bool operator<(const RouterPath &rp) const;
    string getOrderKey() const;
    const string &getPath() const
    {
        return _path;
    }

  protected:

    bool genResult(const string &p, RouterResult& result);

private:
    E_PATH_TYPE _type;
	string		_path;
    string      _stationId;
    int         _orderID;
	regex_t		_reg;
    bool        _isCase;
	
    string      _proxyHost;
    string      _proxyPath;
};
typedef shared_ptr<RouterPath> RouterPathPtr;

class RouterHost
{
public:
	enum E_HOST_TYPE
	{
        EHT_NULL        = 0,
		EHT_FULL 		= 1,
		EHT_PREWCMATCH 	= 2,
		EHT_SUFWCMATCH 	= 3,
		EHT_REGMATCH 	= 4,
		EHT_DEFAULT 	= 99,
	};

    //bool initHost(const map<string, string>& h);
    /**
     * @param: serverName: server_name
     * @param: location: location
     * @param: pp: proxy_pass
     */

    RouterHost(): _type(EHT_NULL) 
    {}

    RouterHost(const string& serverName): _type(EHT_NULL)
    {
        initHost(serverName);
    }

    ~RouterHost()
    {
        if (_type == EHT_REGMATCH)
        {
            regfree(&_reg);
        }
    }

    bool initHost(const string& serverName);
    bool addPath(int id, const string& location, const string& proxyPass, const string& stationId);
    bool matchPath(const string &p, RouterResult& result);
    bool matchHost(const string &h);
    bool operator<(const RouterHost &rh);

    const string &getOrderKey() const
    {
        return _host;
    }

    static RouterHost::E_HOST_TYPE getHostType(const string& serverName);

protected:


private:
    E_HOST_TYPE _type;
	string 		_host;
	regex_t		_reg;

	map<string, RouterPathPtr>		_fullPath;		// 全路径匹配
	set<RouterPathPtr>	_startWithPath;	// 以xx 开头
	set<RouterPathPtr>	_regSuccPath; 	// 正则匹配，是否区分大小写
	set<RouterPathPtr>	_regFailPath; 	// 正则非匹配，也有区分大小写
    set<RouterPathPtr>	_commPath; 	    // 路径通配
	RouterPathPtr	    _defaultPath; 	// 默认路径
};
typedef shared_ptr<RouterHost> RouterHostPtr;

class HttpRouter
{
public:
    struct StrLengthCmp 
    {
        bool operator()(const std::string& k1, const std::string& k2) const
        {
            if (k1.length() > k2.length())
                return true;
            else if (k1.length() < k2.length())
                return false;
            else
                return (k1 < k2);
        }
    };

    typedef map<string, RouterHostPtr, StrLengthCmp> LenMap;
    typedef map<string, RouterHostPtr, StrLengthCmp>::iterator LenMapIterator;

    HttpRouter()
    {
    }

    bool addRouter(int id, const string &serverName, const string &location, const string &proxyPass, const string &stationId);

    bool parse(const string& host, const string& url, RouterResult& result);

private:
    TC_ThreadRWLocker   _rwLock;
	map<string, RouterHostPtr>		_fullHost;		    // 全路径匹配
	LenMap          _preWCMatchHost;	// 前面是通配符, 即后缀匹配
	LenMap          _sufWCMatchHost;	// 后面是通配符, 即前缀匹配
	LenMap 	        _regHost;		    // 正则匹配
	RouterHostPtr   _defaultHost;	    // 默认host
};

class RouterAgent: public TC_Singleton<RouterAgent>
{
public:
    bool parse(const string& host, const string& url, RouterResult& result)
    {
        if (!_router)
        {
            TLOG_ERROR("router is not already!" << endl);
            return false;
        }
        return _router->parse(host, url, result);
    }

    bool reload(const vector<RouterParam> &param);

private:
    shared_ptr<HttpRouter> _router;
};

#define Router RouterAgent::getInstance()

/*
nginx 路由规则，先匹配server_name， 匹配到 server_name 后，再匹配path， 然后根据 proxy_pass 路径进行转发

server_name 匹配逻辑：
{
	1、查找全匹配
	2、通配符在前匹配
	3、通配符在后面匹配
	4、正则匹配
	5、如果server_name为空， 则默认都匹配
}

path匹配逻辑：
{
	1、= 全匹配：  /login
	2、^~ uri以某个常规字符串开头： ^~ /static/   （一旦匹配成功， 不再往后面匹配）
	3、~ 正则匹配(区分大小写)：  ~ \.(gif|jpg|png|js|css)$  （正则表达式匹配多个的情况下， 按最长的匹配）
	4、~* 正则匹配(不区分大小写)：~* \.png$
	5、!~和!~*分别为区分大小写不匹配及不区分大小写不匹配 的正则：  !~ \.xhtml$，  !~* \.xhtml$
	6、/xxx 从头开始匹配路径（匹配长度越长优先级越高）
	7、/ 任何请求都会匹配
}

proxy_pass:
{
	1、如果proxy_pass配置中没有路径（http://host/ 这个是有路径的 /），这时候 location 匹配的完整路径将直接透传给 url 
	2、proxy_pass配置中包含路径（哪怕只有一个 / ， 也算）， 新路径 = proxypassPath + (访问路径-location路径)
	3、当 location 中为正则时， proxy_pass 不能带路径
}
*/

#endif // !1_HTTP_ROUTER_H_
