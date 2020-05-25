#include "HttpServer.h"
#include "HttpImp.h"
//#include "gperftools/profiler.h"

using namespace std;

SimpleHttpServer g_app;

/////////////////////////////////////////////////////////////////
void
SimpleHttpServer::initialize()
{
	//initialize application here:
	//...

	addServant<HttpImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".HttpObj");
	addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".HttpObj", &TC_NetWorkBuffer::parseHttp);

	try
	{
		addConfig(ServerConfig::ServerName + ".conf");
		TC_Config conf;
		conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
		randTimeout = (bool)(TC_Common::strto<int>(conf.get("/main<rand_timeout>", "0")));
	}
	catch(const std::exception& e)
	{
		TLOGERROR("exception:" << e.what() << endl);
	}
}
/////////////////////////////////////////////////////////////////
void
SimpleHttpServer::destroyApp()
{
	//destroy application here:
	//...
}
/////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
	try
	{
		g_app.main(argc, argv);
		g_app.waitForShutdown();
	}
	catch (std::exception& e)
	{
		cerr << "std::exception:" << e.what() << std::endl;
	}
	catch (...)
	{
		cerr << "unknown exception." << std::endl;
	}
	return -1;
}
/////////////////////////////////////////////////////////////////
