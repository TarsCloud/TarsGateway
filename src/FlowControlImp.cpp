#include "FlowControlImp.h"
#include "FlowControlManager.h"

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