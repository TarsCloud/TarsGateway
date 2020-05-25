#include <iostream>
#include "util/tc_http.h"
#include "util/tc_http_async.h"
#include "util/tc_option.h"
#include "util/tc_common.h"
#include "util/tc_clientsocket.h"
#include "util/tc_thread_pool.h"
#include "tup/tup.h"
#include "util/tc_timeprovider.h"
#include "gperftools/profiler.h"

using namespace std;
using namespace tars;
using namespace tup;

struct Param
{
	int buffersize;
	int count;
	string call;
	int thread;
	int flow;
};

Param param;
std::atomic<int> callback_count(0);
std::atomic<int> request_count(0);

void wupSyncCall(int excut_num)
{
	string buffer(param.buffersize, 'a');

	UniPacket<> req;

	req.setRequestId(0);
	req.setServantName("hello");
	req.setFuncName("testHello");

	req.put<string>("sReq", buffer);

	string buff = "";
	req.encode(buff);

	string sServer("http://127.0.0.1:38080/wup");

	TC_HttpRequest stHttpReq;
	stHttpReq.setCacheControl("no-cache");
	stHttpReq.setPostRequest(sServer, buff, true);

	int64_t t = TC_Common::now2us();
	for(int i = 0; i < excut_num; i++)
	{
		TC_HttpResponse stHttpRsp;

		int ret = stHttpReq.doRequest(stHttpRsp, 3000);

//		cout << ret << ":" << recvLen << endl;

		UniPacket<> rsp;

		rsp.decode(stHttpRsp.getContent().c_str(), stHttpRsp.getContent().size());

		string retStr = "";

		ret = rsp.getResultCode();

		rsp.get("sRsp", retStr);

		++callback_count;
	}

	int64_t cost = TC_Common::now2us() - t;
	cout << "wupCall send:" << cost << "us, avg:" << 1.*cost/excut_num << "us" << endl;
}


class AsyncWupHttpCallback : public TC_HttpAsync::RequestCallback
{
public:
	AsyncWupHttpCallback()
	{
	}
	virtual bool onContinue(TC_HttpResponse &stHttpResponse) { return true; }
	virtual void onSucc(TC_HttpResponse &stHttpRsp)
	{
		UniPacket<> rsp;

		rsp.decode(stHttpRsp.getContent().c_str(), stHttpRsp.getContent().size());

		string retStr = "";

		int ret = rsp.getResultCode();

		rsp.get("sRsp", retStr);

		++callback_count;
	}

	virtual void onFailed(FAILED_CODE ret, const string &info)
	{
		cout << "onFailed, code:" << ret << ", info:" << info << ", " << request_count - callback_count << endl;
	}
	virtual void onClose()
	{
//		cout << "onClose:" << endl;
	}
};

TC_HttpAsync ast;

void wupAsyncCall(int excut_num)
{
	uint64_t i = TC_Common::now2ms();

	string buffer(param.buffersize, 'a');

	UniPacket<> req;

	req.setRequestId(0);
	req.setServantName("hello");
	req.setFuncName("testHello");

	req.put<string>("sReq", buffer);

	string buff = "";
	req.encode(buff);

	string sServer("http://127.0.0.1:38080/wup");

	TC_HttpRequest stHttpReq;
	stHttpReq.setPostRequest(sServer, buff, true);

	int64_t t = TC_Common::now2us();
	for(int i = 0; i < excut_num; )
	{
		if((request_count - callback_count) <= param.flow/100)
		{
			i++;
			
			// cout << i - callback_count << ", " << (param.flow/10) << endl;
			++request_count;
			TC_HttpAsync::RequestCallbackPtr p = new AsyncWupHttpCallback();
			ast.doAsyncRequest(stHttpReq, p);
		}
		else
		{
			TC_Common::msleep(10);
		}
	}

	int64_t cost = TC_Common::now2us() - t;
	cout << "wupCall send:" << cost << "us, avg:" << 1.*cost/excut_num << "us" << endl;
}


class AsyncHttpCallback : public TC_HttpAsync::RequestCallback
{
public:
	AsyncHttpCallback()
	{
	}
	virtual bool onContinue(TC_HttpResponse &stHttpResponse) { return true; }
	virtual void onSucc(TC_HttpResponse &stHttpRsp)
	{
		++callback_count;
	}

	virtual void onFailed(FAILED_CODE ret, const string &info)
	{
		cout << "onFailed, code:" << ret << ", info:" << info << ", " << request_count - callback_count << endl;
	}
	virtual void onClose()
	{
//		cout << "onClose:" << endl;
	}
};

void httpAsyncCall(int excut_num)
{
	uint64_t i = TC_Common::now2ms();

	string buffer(param.buffersize, 'a');

	string sServer("http://127.0.0.1:38080/hello/abc");

	TC_HttpRequest stHttpReq;
	stHttpReq.setGetRequest(sServer, true);

	int64_t t = TC_Common::now2us();
	for(int i = 0; i < excut_num; )
	{
		if((request_count - callback_count) <= param.flow/100)
		{
			i++;
			
			// cout << i - callback_count << ", " << (param.flow/10) << endl;
			++request_count;
			TC_HttpAsync::RequestCallbackPtr p = new AsyncHttpCallback();
			ast.doAsyncRequest(stHttpReq, p);
		}
		else
		{
			TC_Common::msleep(10);
		}
	}

	int64_t cost = TC_Common::now2us() - t;
	cout << "httpAsyncCall send:" << cost << "us, avg:" << 1.*cost/excut_num << "us" << endl;
}

// http://127.0.0.1:38080/hello/abc
void httpCall(int excut_num)
{
	int64_t t = TC_Common::now2us();

	string buffer(param.buffersize, 'a');

	string sServer1("http://127.0.0.1:38080/");

	TC_HttpRequest stHttpReq;
	stHttpReq.setCacheControl("no-cache");
	stHttpReq.setPostRequest(sServer1, buffer, true);

	TC_TCPClient client ;
	client.init("127.0.0.1", 8080, 3000);

	int iRet = 0;

	for (int i = 0; i<excut_num; i++)
	{
		TC_HttpResponse stHttpRsp;

		iRet = stHttpReq.doRequest(stHttpRsp, 3000);

		if (iRet != 0)
		{
			cout <<"pthread id: " << TC_Thread::CURRENT_THREADID() << ", iRet:" << iRet <<endl;
		}

		++callback_count;
	}

	int64_t cost = TC_Common::now2us() - t;

	cout << "httpCall send:" << cost << "us, avg:" << 1.*cost/excut_num << "us" << endl;
}

void jsonCall(int excut_num)
{

}

int main(int argc, char *argv[])
{
	ProfilerStart("HttpClient.prof");
    try
    {
        if (argc < 4)
        {
	        cout << "Usage:" << argv[0] << "--count=1000 --call=[wupsync|wupasync|http|httpasync|json] --thread=1 --flow=1000 --buffersize=1000" << endl;

	        return 0;
        }

	    TC_Option option;
        option.decode(argc, argv);

		param.count = TC_Common::strto<int>(option.getValue("count"));
	    if(param.count <= 0) param.count = 1000;

	    param.call = option.getValue("call");
	    if(param.call.empty()) param.call = "sync";

	    param.thread = TC_Common::strto<int>(option.getValue("thread"));
	    if(param.thread <= 0) param.thread = 1;

	    param.flow = TC_Common::strto<int>(option.getValue("flow"));
	    if(param.flow <= 0) param.flow = 100;

	    param.buffersize = TC_Common::strto<int>(option.getValue("buffersize"));
	    if(param.buffersize <= 0) param.buffersize = 1;

        int64_t start = TC_Common::now2us();

        std::function<void(int)> func;

	    ast.setTimeout(10000);
	    ast.start();

	    if (param.call == "wupsync")
        {
            func = wupSyncCall;
        }
        else if (param.call == "wupasync")
        {
	        func = wupAsyncCall;
        }
        else if (param.call == "httpasync")
        {
            func = httpAsyncCall;
        }
        else if (param.call == "json")
        {
	        func = jsonCall;
        }
        else
        {
        	cout << "no func, exits:" << param.call << endl;
        	exit(0);
        }

	    vector<std::thread*> vt;
        for(int i = 0 ; i< param.thread; i++)
        {
            vt.push_back(new std::thread(func, param.count));
        }

        std::thread print([&]{while(callback_count != param.count * param.thread) {
	        cout << param.call << ": request count:" << request_count << ", finish count:" << callback_count << endl;
	        std::this_thread::sleep_for(std::chrono::seconds(1));
        };});

        for(size_t i = 0 ; i< vt.size(); i++)
        {
            vt[i]->join();
            delete vt[i];
        }

        cout << "(pid:" << std::this_thread::get_id() << ")"
             << "(count:" << param.count << ")"
             << "(use ms:" << (TC_Common::now2us() - start)/1000 << ")"
             << endl;

	    while(callback_count != param.count * param.thread) {
		    std::this_thread::sleep_for(std::chrono::seconds(1));
	    }
	    print.join();
	    cout << "----------finish count:" << callback_count << endl;

	    ast.waitForAllDone();
    }
    catch(exception &ex)
    {
        cout << ex.what() << endl;
    }
    cout << "main return." << endl;

	ProfilerStop();

    return 0;
}