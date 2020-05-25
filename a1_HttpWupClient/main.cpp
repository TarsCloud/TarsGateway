#include "util/tc_common.h"
#include "util/tc_http.h"
#include <sys/un.h>
#include <iostream>
#include "wup-linux-c++/wup.h"
#include "util/tc_option.h"
#include "Login.h"

//#include "GoldAccount.h"
//#include "hello.h"

using namespace std;
using namespace tars;
using namespace wup;
using namespace Base;
//using namespace Trade;

TC_Option g_op;

void addOnePacket(string& buffer, size_t id)
{
    UniPacket<> req;

    req.setRequestId(id);
    req.setServantName("hello");
    req.setFuncName("testHello");

    req.put<string>("s", "test-" + TC_Common::tostr(id));

    string tmp;

    req.encode(tmp);

    buffer += tmp;
}

int parsePacket(const char *buffer, size_t length)
{
    size_t pos = 0;

    while (pos < length)
    {
        //长度保护
        if (pos + sizeof(uint32_t) > length)
        {
            cout << "wup error: " << pos << "+4 > " << length << endl;
            return -1;
        }

        uint32_t l = 0;
        memcpy(&l, buffer + pos, sizeof(uint32_t));
        l = ntohl(l);

        //长度保护
        if (pos + l > length)
        {
            cout << "wup error: " << pos + l << " > " << length << endl;
            return -2;
        }

        if (l <= 4)
        {
            cout << "wup error: l:" << l << " <= 4 , pos:" << pos << ", length:" << length << endl;
            return -3;
        }

        UniPacket<> rsp;

        rsp.decode(buffer + pos + 4, l - 4);

        string retStr = "";

        int ret = rsp.getResultCode();

        rsp.get("r", retStr);

        cout << rsp.getServantName() << "::"
            << rsp.getFuncName() << ", requestid:"
            << rsp.getRequestId() << ", "
            << "ret:" << ret << ", r:" << retStr << endl;

        pos += l;
    }

    return 0;
}

int parseGetSum(const char *buffer, size_t length)
{
	size_t pos = 0;

	while (pos < length)
	{
		//长度保护
		if (pos + sizeof(uint32_t) > length)
		{
			cout << "wup error: " << pos << "+4 > " << length << endl;
			return -1;
		}

		uint32_t l = 0;
		memcpy(&l, buffer + pos, sizeof(uint32_t));
		l = ntohl(l);

		//长度保护
		if (pos + l > length)
		{
			cout << "wup error: " << pos + l << " > " << length << endl;
			return -2;
		}

		if (l <= 4)
		{
			cout << "wup error: l:" << l << " <= 4 , pos:" << pos << ", length:" << length << endl;
			return -3;
		}

		UniPacket<> rsp;

		rsp.decode(buffer + pos + 4, l - 4);

		int sum = 0;

		int ret = rsp.getResultCode();

		rsp.get("sum", sum);

		cout << rsp.getServantName() << "::"
			<< rsp.getFuncName() << ", requestid:"
			<< rsp.getRequestId() << ", "
			<< "ret:" << ret << ", sum:" << sum << endl;

		pos += l;
	}

	return 0;
}

/*
int parseGetApplyIdRsp(const char *buffer, size_t length)
{
	size_t pos = 0;

	while (pos < length)
	{
		//长度保护
		if (pos + sizeof(uint32_t) > length)
		{
			cout << "wup error: " << pos << "+4 > " << length << endl;
			return -1;
		}

		uint32_t l = 0;
		memcpy(&l, buffer + pos, sizeof(uint32_t));
		l = ntohl(l);

		//长度保护
		if (pos + l > length)
		{
			cout << "wup error: " << pos + l << " > " << length << endl;
			return -2;
		}

		if (l <= 4)
		{
			cout << "wup error: l:" << l << " <= 4 , pos:" << pos << ", length:" << length << endl;
			return -3;
		}

		UniPacket<> rsp;

		rsp.decode(buffer + pos + 4, l - 4);

		ApplyAccountIdRsp applyRsp;

		int ret = rsp.getResultCode();

		rsp.get("rsp", applyRsp);

		cout << rsp.getServantName() << "::"
			<< rsp.getFuncName() << ", requestid:"
			<< rsp.getRequestId() << ", "
			<< "ret:" << ret << ", applyId:" << applyRsp.applyId << endl;

		pos += l;
	}

	return 0;
}

int testGetApplyId(int argc, char *argv[])
{
	string wupUrl = string(argv[1]);
	int round = TC_Common::strto<int>(string(argv[2]));

	if (wupUrl == "hz")
	{
		wupUrl = "http://prx.upchina.com";
	}
	else if (wupUrl == "sz")
	{
		wupUrl = "http://sz.prx.upchina.com";
	}
	else if (wupUrl == "uptest")
	{
		wupUrl = "http://121.14.69.14:18080";
	}
	else if (wupUrl.length() < 10 || wupUrl.substr(0, 4) != "http")
	{
		cout << "Use:" << argv[1] << " Wup-Address" << endl;
		return -1;
	}

	cout << "Total round:" << round << ", wup url:" << wupUrl << endl;

	while (round -- > 0)
	{
		UniPacket<> req;
		//req.setVersion(2);
		req.setRequestId(1);

		req.setServantName("goldaccount");
		req.setFuncName("getApplyId");

		ApplyAccountIdReq applyReq;
		applyReq.baseInfo.guid = "12345678123456781234567812345678";
		applyReq.baseInfo.xua = "Test_com.test.xxx";
		applyReq.baseInfo.featureCode = "testcode";
		applyReq.baseInfo.phoneNumber = "13000000000";
		applyReq.baseInfo.phoneMac = "AB:CD:12:34";

		req.put<ApplyAccountIdReq>("req", applyReq);

		string buff;

		req.encode(buff);

		//cout << TC_Common::bin2str(buff.c_str(), buff.size()) << endl;

		TC_HttpRequest stHttpReq;
		stHttpReq.setCacheControl("no-cache");
		stHttpReq.setContent(buff, true);
		stHttpReq.setPostRequest(wupUrl, buff.c_str(), buff.size());

		TC_HttpResponse stHttpRep;
		int iRet = stHttpReq.doRequest(stHttpRep, 30000);
		if (iRet != 0)
		{
			cout << iRet << endl;
		}

		string buffer = stHttpRep.getContent();

		cout << "[" << round + 1 << "] rsp body len:" << buffer.length() << endl;

		parseGetApplyIdRsp(buffer.c_str(), buffer.length());
	}
	

	return 0;
}
*/

int testGetSum(int argc, char *argv[])
{
	string wupUrl = string(argv[1]);
	//int round = TC_Common::strto<int>(string(argv[2]));
	int round = 1;

	if (wupUrl == "hz")
	{
		wupUrl = "http://prx.upchina.com";
	}
	else if (wupUrl == "sz")
	{
		wupUrl = "http://sz.prx.upchina.com";
	}
	else if (wupUrl == "uptest")
	{
		wupUrl = "http://121.14.69.14:18080";
	}
	else if (wupUrl.length() < 10 || wupUrl.substr(0, 4) != "http")
	{
		cout << "Use:" << argv[1] << " Wup-Address" << endl;
		return -1;
	}

	cout << "Total round:" << round << ", wup url:" << wupUrl << endl;

	while (round-- > 0)
	{
		UniPacket<> req;
		//req.setVersion(2);
		req.setRequestId(1);

		req.setServantName("testGetSum");
		req.setFuncName("testGetSum");

		req.put<int>("x", 15);
		req.put<int>("y", 108);

		string buff;

		req.encode(buff);

		cout << TC_Common::bin2str(buff.c_str(), buff.size()) << endl;

		TC_HttpRequest stHttpReq;
		stHttpReq.setCacheControl("no-cache");
		stHttpReq.setContent(buff, true);
		stHttpReq.setPostRequest(wupUrl, buff.c_str(), buff.size());

		TC_HttpResponse stHttpRep;
		int iRet = stHttpReq.doRequest(stHttpRep, 3000);
		if (iRet != 0)
		{
			cout << iRet << endl;
		}

		string buffer = stHttpRep.getContent();

		cout << "[" << round+1 << "] rsp body len:" << buffer.length() << endl;

		parseGetSum(buffer.c_str(), buffer.length());
	}	

	return 0;
}

/*
int parseHelloRsp(const char *buffer, size_t length)
{
	size_t pos = 0;

	while (pos < length)
	{
		//长度保护
		if (pos + sizeof(uint32_t) > length)
		{
			cout << "wup error: " << pos << "+4 > " << length << endl;
			return -1;
		}

		uint32_t l = 0;
		memcpy(&l, buffer + pos, sizeof(uint32_t));
		l = ntohl(l);

		//长度保护
		if (pos + l > length)
		{
			cout << "wup error: " << pos + l << " > " << length << endl;
			return -2;
		}

		if (l <= 4)
		{
			cout << "wup error: l:" << l << " <= 4 , pos:" << pos << ", length:" << length << endl;
			return -3;
		}

		UniPacket<> rsp;

		rsp.decode(buffer + pos + 4, l - 4);

		TestApp::HelloRsp hRsp;

		int ret = rsp.getResultCode();

		rsp.get("stRsp", hRsp);

		cout << rsp.getServantName() << "::"
			<< rsp.getFuncName() << ", requestid:"
			<< rsp.getRequestId() << ", "
			<< "ret:" << ret << ", rsp:" << hRsp.msg << endl;

		pos += l;
	}

	return 0;
}

int testHello(int argc, char *argv[])
{
	string wupUrl = g_op.getValue("url");

	int round = 1;

	if (wupUrl == "hz")
	{
		wupUrl = "http://prx.upchina.com";
	}
	else if (wupUrl == "sz")
	{
		wupUrl = "http://sz.prx.upchina.com";
	}
	else if (wupUrl == "test")
	{
		wupUrl = "http://172.16.8.124:18080";
	}
	else if (wupUrl == "uptest")
	{
		wupUrl = "http://121.14.69.14:18080";
	}
	else if (wupUrl.length() < 10 || wupUrl.substr(0, 4) != "http")
	{
		cout << "Use:" << argv[1] << " Wup-Address" << endl;
		return -1;
	}

	cout << "Total round:" << round << ", wup url:" << wupUrl << endl;

	int requestId = 1;
	while (round-- > 0)
	{
		UniPacket<> req;
		req.setVersion(2);
		req.setRequestId(requestId++);

		//req.setServantName("TestApp.HelloServer.HelloObj");
		req.setServantName("javahello");
		req.setFuncName("hello");

		TestApp::HelloReq hReq;
		hReq.s = 2018;
		hReq.name = "HelloWorld";

		req.put<TestApp::HelloReq>("stReq", hReq);	

		string buff;

		req.encode(buff);

		cout << TC_Common::bin2str(buff.c_str(), buff.size()) << endl;

		TC_HttpRequest stHttpReq;
		stHttpReq.setCacheControl("no-cache");
		stHttpReq.setContent(buff, true);
		stHttpReq.setPostRequest(wupUrl, buff.c_str(), buff.size());

		TC_HttpResponse stHttpRep;
		int iRet = stHttpReq.doRequest(stHttpRep, 3000);
		if (iRet != 0)
		{
			cout << iRet << endl;
		}

		string buffer = stHttpRep.getContent();

		cout << "[" << round + 1 << "] rsp body len:" << buffer.length() << endl;

		parseHelloRsp(buffer.c_str(), buffer.length());
	}

	return 0;
}
*/

int parseLoginRsp(const char *buffer, size_t length)
{
	size_t pos = 0;

	while (pos < length)
	{
		//长度保护
		if (pos + sizeof(uint32_t) > length)
		{
			cout << "wup error: " << pos << "+4 > " << length << endl;
			return -1;
		}

		uint32_t l = 0;
		memcpy(&l, buffer + pos, sizeof(uint32_t));
		l = ntohl(l);

		//长度保护
		if (pos + l > length)
		{
			cout << "wup error: " << pos + l << " > " << length << endl;
			return -2;
		}

		if (l <= 4)
		{
			cout << "wup error: l:" << l << " <= 4 , pos:" << pos << ", length:" << length << endl;
			return -3;
		}

		UniPacket<> rsp;

		rsp.decode(buffer + pos + 4, l - 4);

		Base::LoginRsp hRsp;

		int ret = rsp.getResultCode();

		rsp.get("o", hRsp);

		cout << rsp.getServantName() << "::"
			<< rsp.getFuncName() << ", requestid:"
			<< rsp.getRequestId() << ", "
			<< "ret:" << ret << ", rsp:" << TC_Common::bin2str(&hRsp.vGUID[0], hRsp.vGUID.size())
		    << endl;

		for (auto &p : hRsp.vInfoList)
		{
			cout << "type:" << p.eIpType << "|" << TC_Common::tostr(p.vIPList) << "|" << TC_Common::tostr(p.vServantList) << endl;
		}

		pos += l;
	}

	return 0;
}

int testLogin(int argc, char *argv[])
{
	string wupUrl = g_op.getValue("url");

	int round = 1;

	if (wupUrl == "hz")
	{
		wupUrl = "http://prx.upchina.com";
	}
	else if (wupUrl == "sz")
	{
		wupUrl = "http://sz.prx.upchina.com";
	}
	else if (wupUrl == "test")
	{
		wupUrl = "http://172.16.8.124:18080";
	}
	else if (wupUrl == "uptest")
	{
		wupUrl = "http://121.14.69.14:18080";
	}
	else if (wupUrl.length() < 10 || wupUrl.substr(0, 4) != "http")
	{
		cout << "Use:" << argv[1] << " Wup-Address" << endl;
		return -1;
	}

	cout << "Total round:" << round << ", wup url:" << wupUrl << endl;

	int requestId = 1;
	while (round-- > 0)
	{
		UniPacket<> req;
		req.setVersion(2);
		req.setRequestId(requestId++);

		//req.setServantName("TestApp.HelloServer.HelloObj");
		req.setServantName("login");
		req.setFuncName("login");

		Base::LoginReq lReq;
		lReq.stBaseInfo.sXUA = "WIN_com.hummer.changjiang.client.windows&VN=0_1.0.3_530_DD&RL=1920_1080&OS=6.1.7601&CHID=4601_4501&MN=hummer&SDK=1.0.1&MO=&VC=&RV=";
		lReq.cLoginVer = 1;
		req.put<Base::LoginReq>("i", lReq);

		string buff;

		req.encode(buff);

		cout << TC_Common::bin2str(buff.c_str(), buff.size()) << endl;

		TC_HttpRequest stHttpReq;
		stHttpReq.setCacheControl("no-cache");
		stHttpReq.setContent(buff, true);
		stHttpReq.setPostRequest(wupUrl, buff.c_str(), buff.size());

		TC_HttpResponse stHttpRep;
		int iRet = stHttpReq.doRequest(stHttpRep, 3000);
		if (iRet != 0)
		{
			cout << iRet << endl;
		}

		string buffer = stHttpRep.getContent();

		cout << "[" << round + 1 << "] rsp body len:" << buffer.length() << endl;

		cout << "rsp wup data:" << TC_Common::bin2str(buffer, " ", 16) << endl;
		parseLoginRsp(buffer.c_str(), buffer.length());
	}

	return 0;
}

int main(int argc, char *argv[])
{
    try
    {
		g_op.decode(argc, argv);
		if (g_op.getValue("cmd") == "hello")
		{
			//testHello(argc, argv);
		}
		else if (g_op.getValue("cmd") == "login")
		{
			testLogin(argc, argv);
		}

		/*if (argc == 3)
		{
			return testGetApplyId(argc, argv);
		}

		if (argc == 2)
		{
			return testGetSum(argc, argv);
		}		

        string buff;

        addOnePacket(buff, 0);
        addOnePacket(buff, 1);
        addOnePacket(buff, 2);

        cout << TC_Common::bin2str(buff.c_str(), buff.size()) << endl;

        TC_HttpRequest stHttpReq;
        stHttpReq.setCacheControl("no-cache");
        stHttpReq.setContent(buff, true);
        stHttpReq.setPostRequest("http://120.25.228.209:443", buff.c_str(), buff.size());

        TC_HttpResponse stHttpRep;
        int iRet = stHttpReq.doRequest(stHttpRep, 3000);
        if (iRet != 0)
        {
            cout << iRet << endl;
        }

        cout << TC_Common::tostr(stHttpRep.getHeaders()) << endl;

        string recvBuff = stHttpRep.getContent();

        parsePacket(recvBuff.c_str(), recvBuff.length());*/
    }
    catch (exception& ex)
    {
        cout << ex.what() << endl;
    }
    cout << "main return." << endl;

    return 0;
}
