#include "VerifyImp.h"
#include "servant/Application.h"
#include "util/tc_hash_fun.h"

using namespace std;

//////////////////////////////////////////////////////
void VerifyImp::initialize()
{
	//initialize servant here:
	//...
}

//////////////////////////////////////////////////////
void VerifyImp::destroy()
{
	//destroy servant here:
	//...
}

tars::Int32 VerifyImp::verify(const Base::VerifyReq & req, Base::VerifyRsp &rsp, tars::CurrentPtr current)
{
	TLOG_DEBUG("token:" << req.token << "|verifyHeaders:" << TC_Common::tostr(req.verifyHeaders) << "|data size:" << req.body.size() << endl);
	if (req.token.length() % 4 == 0)
	{
		rsp.ret = EVC_SUCC;
		rsp.uid = tars::TC_Common::tostr(tars::hash<string>()(req.token));
		if (!req.body.empty())
		{
			rsp.context = "test-verify-data";
		}
		TLOG_DEBUG("ret:" << rsp.ret << "|uid:" << rsp.uid << "|data:" << rsp.context << endl);
		return 0;
	}
	else if (req.token.length() % 4 == 1)
	{
		rsp.ret = EVC_SYS_ERR;		
	}
	else if (req.token.length() % 4 == 2)
	{
		rsp.ret = EVC_ERR_TOKEN;
	}
	else if (req.token.length() % 4 == 3)
	{
		rsp.ret = EVC_TOKEN_EXPIRE;
	}
	TLOG_ERROR("ret:" << rsp.ret << endl);
	return -1;
}