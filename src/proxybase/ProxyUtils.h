#ifndef _PROXYUTILS_H_
#define _PROXYUTILS_H_

#include "servant/Application.h"

using namespace tars;

class ProxyUtils
{
public:

    // 统一错误返回处理
    static string getHttpErrorRsp(int statusCode)
    {
        TC_HttpResponse httpRsp;
        string content;
        string info;
        if (400 == statusCode)
        {
            info = "Bad Request";
            content = "<html> <head><title>400 Bad Request</title></head> <body> <center><h1>400 Bad Request</h1></center> </body> </html>";
        }
        else if (401 == statusCode)
        {
            info = "Unauthorized";
            content = "<html> <head><title>401 Unauthorized</title></head> <body> <center><h1>401 Unauthorized</h1></center> </body> </html>"; 
        }
        else if (403 == statusCode)
        {
            info = "Forbidden";
            content = "<html> <head><title>403 Forbidden</title></head> <body> <center><h1>403 Forbidden</h1></center> </body> </html>"; 
        }
        else if (404 == statusCode)
        {
            info = "Not Found";
            content = "<html> <head><title>404 Not Found</title></head> <body> <center><h1>404 Not Found</h1></center> </body> </html>";
        }
        else if (429 == statusCode)
        {
            info = "Too Many Request";
            content = "<html> <head><title>429 Too Many Request</title></head> <body> <center><h1>429 Too Many Request</h1></center> </body> </html>";
        }
        else if (500 == statusCode)
        {
            info = "Server Interval Error";
            content = content = "<html> <head><title>500 Server Interval Error</title></head> <body> <center><h1>500 Server Interval Error</h1></center> </body> </html>";
        }
        else if (501 == statusCode)
        {
            info = "Not Implemented";
            content = "<html> <head><title>501 Not Implemented</title></head> <body> <center><h1>501 Not Implemented</h1></center> </body> </html>";
        }
        else if (502 == statusCode)
        {
            info = "Bad Gateway";
            content = "<html> <head><title>502 Bad Gateway</title></head> <body> <center><h1>502 Bad Gateway</h1></center> </body> </html>";
        }
        else if(503 == statusCode)
        {
            info = "Service Unavailable";
            content = "<html> <head><title>503 Service Unavailable</title></head> <body> <center><h1>503 Service Unavailable</h1></center> </body> </html>";
        }
        else if(504 == statusCode)
        {
            info = "Gateway Timeout";
            content = "<html> <head><title>504 Gateway Timeout</title></head> <body> <center><h1>504 Gateway Timeout</h1></center> </body> </html>"; 
        }
        else
        {
            info = "Server Interval Error";
            content = "<html> <head><title>Server Interval Error</title></head> <body> <center><h1>Server Interval Error</h1></center> </body> </html>";
        }
        
        httpRsp.setResponse(statusCode, info, content);
        return httpRsp.encode();
    }

    static void doErrorRsp(int statusCode, tars::TarsCurrentPtr current, bool keepAlive = false)
    {
        string data = getHttpErrorRsp(statusCode);
        current->sendResponse(data.c_str(), data.length());
        if (!keepAlive)
        {
            current->close();
        }

        TLOG_DEBUG(statusCode << "|" << data << endl);
    }

};


#endif