#ifndef _VerifyDemoServer_H_
#define _VerifyDemoServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class VerifyDemoServer : public Application
{
public:
	/**
	 *
	 **/
	virtual ~VerifyDemoServer() {};

	/**
	 *
	 **/
	virtual void initialize();

	/**
	 *
	 **/
	virtual void destroyApp();
};

extern VerifyDemoServer g_app;

////////////////////////////////////////////
#endif
