#pragma once

#include "servant/Application.h"

/**
 *
 *
 */
class HttpImp : public tars::Servant
{
public:
	/**
	 *
	 */
	virtual ~HttpImp() {}

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
    int doRequest(tars::TarsCurrentPtr current, vector<char> &response);
};
/////////////////////////////////////////////////////

