#pragma once

#include "servant/Application.h"
#include "Verify.h"

using namespace Base;
/**
 *
 *
 */
class VerifyImp : public Base::Verify
{
public:
	/**
	 *
	 */
	virtual ~VerifyImp() {}

	/**
	 *
	 */
	virtual void initialize();

	/**
	 *
	 */
    virtual void destroy();

	/**
	 *
	 */
	tars::Int32 verify(const Base::VerifyReq & req,Base::VerifyRsp &rsp,tars::CurrentPtr current);
};
/////////////////////////////////////////////////////

