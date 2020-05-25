#include "HttpImp.h"
#include "servant/Application.h"
#include "HttpServer.h"

using namespace std;

//////////////////////////////////////////////////////
void HttpImp::initialize()
{
	//initialize servant here:
	//...
}

//////////////////////////////////////////////////////
void HttpImp::destroy()
{
	//destroy servant here:
	//...
	srand(time(NULL));
}

int HttpImp::doRequest(tars::TarsCurrentPtr current, vector<char>& response)
{
    const vector<char>& request = current->getRequestBuffer();

    string sReqContent(request.data(), request.size());

    TLOGDEBUG("HttpImp::doRequest\r\n" << sReqContent.substr(0, sReqContent.find("\r\n\r\n")) << endl);

    TC_HttpRequest req;
    req.decode(sReqContent);
    TLOGDEBUG("url:" << req.getOriginRequest() << endl);

    TC_HttpResponse rsp;
    string data = "<html>hello server:" + TC_Common::now2str() + ", " + req.getOriginRequest() + "!</html>";
    rsp.setResponse(200, "OK", data);
    rsp.setContentType("text/html;charset=utf-8");
    rsp.setContentLength(data.length());
    //    rsp.setConnection("close");
    string buffer = rsp.encode();

	if (g_app.randTimeout)
	{
		unsigned int t = rand() % 20;
		TLOGDEBUG("sleep second:" << t << endl);
		sleep(t);
	}
	current->sendResponse(buffer.c_str(), (uint32_t)buffer.length());
	current->setResponse(false);
	//current->close();
    return 0;
}
