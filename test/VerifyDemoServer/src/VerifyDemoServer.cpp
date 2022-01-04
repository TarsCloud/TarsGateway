#include "VerifyDemoServer.h"
#include "VerifyImp.h"

using namespace std;

VerifyDemoServer g_app;

/////////////////////////////////////////////////////////////////
void
VerifyDemoServer::initialize()
{
	//initialize application here:
	//...

	addServant<VerifyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".VerifyObj");
}
/////////////////////////////////////////////////////////////////
void
VerifyDemoServer::destroyApp()
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
