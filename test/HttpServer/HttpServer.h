#ifndef _SimpleHttpServer_H_
#define _SimpleHttpServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class SimpleHttpServer : public Application
{
public:
	/**
	 *
	 **/
	virtual ~SimpleHttpServer() {};

	/**
	 *
	 **/
	virtual void initialize();

	/**
	 *
	 **/
	virtual void destroyApp();

	bool 	randTimeout;
};

extern SimpleHttpServer g_app;

////////////////////////////////////////////
#endif
