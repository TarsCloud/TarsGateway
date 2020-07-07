#include "FlowControlImp.h"
#include "proxybase/FlowControlManager.h"

FlowControlImp::FlowControlImp(/* args */)
{
}

FlowControlImp::~FlowControlImp()
{
}

void FlowControlImp::initialize()
{

}

void FlowControlImp::destroy()
{
    
}

int FlowControlImp::report(const map<string, int>& flow, const string& ip, tars::TarsCurrentPtr current)
{
    FlowControlManager::getInstance()->report(flow, ip);
    return 0;
}

int FlowControlImp::getGWDB(map<string, string>& dbConf, tars::TarsCurrentPtr current)
{
    return FlowControlManager::getInstance()->getDBConf(dbConf);
}
