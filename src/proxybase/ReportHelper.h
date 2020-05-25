#ifndef _REPORTHELPER_H_
#define _REPORTHELPER_H_

#include "servant/Application.h"
using namespace tars;

class ReportHelper
{
public:
    //服务调用上报接口
    static void reportStat(const string& sMaster, const string& sSlave, const string& sInterface, int iRet= 0, int iTotaltime= 0, const string& slaveIp = "")
    {
        tars::CommunicatorPtr pComm = tars::Application::getCommunicator();
        if (!pComm)
        {
            return;
        }

        StatReport *pStatReport = pComm->getStatReport();
        if (!pStatReport)
        {
            return;
        }

        string sSlaveIP = (slaveIp.length() > 0 ? slaveIp : ServerConfig::LocalIp);
        if (iRet == -7)
        {
            pStatReport->report(sMaster, ServerConfig::LocalIp, ServerConfig::Application + "." + sSlave, sSlaveIP,
                                iRet, sInterface,  StatReport::STAT_TIMEOUT, iTotaltime, 0);
        }
        else if (iRet == 0)
        {
            pStatReport->report(sMaster, ServerConfig::LocalIp, ServerConfig::Application + "." + sSlave, sSlaveIP,
                                iRet, sInterface,  StatReport::STAT_SUCC, iTotaltime, 0);
        }
        else
        {
            pStatReport->report(sMaster, ServerConfig::LocalIp, ServerConfig::Application + "." + sSlave, sSlaveIP,
                                iRet, sInterface, StatReport::STAT_EXCE, iTotaltime, 0);
        }
    }

    //属性上报接口
    static void reportProperty(const string& sProperty, int iValue= 1, int iType= 0)
    {
        tars::CommunicatorPtr pComm = tars::Application::getCommunicator();
        if (!pComm)
        {
            return;
        }

        StatReport *pStatReport = pComm->getStatReport();
        if (!pStatReport)
        {
            return;
        }

        string sPropertyKey = ServerConfig::Application + "." + ServerConfig::ServerName + "." + sProperty;
        PropertyReportPtr pPropertyReport = pStatReport->getPropertyReport(sPropertyKey);
        if (!pPropertyReport)
        {
            if (iType == 0) // count
            {
                pPropertyReport = pStatReport->createPropertyReport(sPropertyKey, PropertyReport::count());
            }
            else if (iType == 1) // 求和
            {
                pPropertyReport = pStatReport->createPropertyReport(sPropertyKey, PropertyReport::sum());
            }
            else if (iType == 2) // 求平均
            {
                pPropertyReport = pStatReport->createPropertyReport(sPropertyKey, PropertyReport::avg());
            }
            else
            {
                pPropertyReport = pStatReport->createPropertyReport(sPropertyKey, PropertyReport::count());
            }
        }
        if (pPropertyReport)
        {
            pPropertyReport->report(iValue);
        }
    }
};

#endif // !1_REPORTHELPER_H_
