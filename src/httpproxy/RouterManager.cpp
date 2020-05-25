#include "RouterManager.h"
#include "StationManager.h"

RouterManager::RouterManager()
{
    //_lastUpdateTime = 0;
    _maxDepth = 0;
}

void RouterManager::updateRouter(const unordered_map<string, string> &router, const string& ver)
{
    if (ver == _ver)
    {
        return;
    }
    map<size_t, set<string>> routerDepth;
    unordered_map<string, string> routerNew;
    size_t maxDepth = 0;
    for (auto it = router.begin(); it != router.end(); ++it)
    {
        string u = it->first;

        if (u.substr(0, 7) != "http://" && u.substr(0, 8) != "https://")
        {
            u = "http://" + u;
        }
        if (u[u.length()-1] != '/')
        {
            u += "/";
        }

        routerDepth[u.size()].insert(u);
        routerNew[u] = it->second;

        if (u.length() > maxDepth)
        {
            maxDepth = u.length();
        }
        FDLOG("load") << FILE_FUNC_LINE << "add_router|" << it->second << "|" << it->first << "|" << u << "|" << u.length() << endl;
    }

    {
        TC_ThreadWLock w(_rwLock);
        _router.swap(routerNew);
        _routerDepth.swap(routerDepth);
        _maxDepth = maxDepth;
        _ver = ver;
    }
}

bool RouterManager::getStation(const string& url, string& station, string& funcPath)
{
    TC_ThreadRLock r(_rwLock);
    string u = url.substr(0, _maxDepth);
    //TLOGDEBUG(url << "|after substr:" << u << endl);
    for (size_t i = u.length(); i > 7; i--)
    {
        if (u[i-1] == '/')
        {
            auto it = _routerDepth.find(i);
            if (it == _routerDepth.end())
            {
                continue;
            }
            string uTmp = u.substr(0, i);
            //TLOGDEBUG("find /, i:" << i << "|" << uTmp << endl);
            auto itS = it->second.find(uTmp);
            if (itS != it->second.end())
            {
                auto itStation = _router.find(uTmp);
                if (itStation != _router.end())
                {
                    station = itStation->second;
                    string::size_type pos = url.find("/", i+1);
                    if (pos != string::npos)
                    {
                        funcPath = url.substr(i, pos-i);
                    }
                    else if (url.length() > i)
                    {
                        funcPath = url.substr(i, url.length() - i);
                    }
                    else
                    {
                        funcPath = "default_root";
                    }

                    break;
                }
            }
        
        }
    }

    TLOGDEBUG(url << "===>find station:" << station << ", func path:" << funcPath << endl);

    return (station.length() > 0);
}