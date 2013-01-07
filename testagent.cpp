
#include "agentsrv.h"

int main()
{
	AgentSrv srv("0.0.0.0:7777");
	srv.Start();
	srv.Run();
	srv.Stop();
	return 0;
}
