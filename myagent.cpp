// myagent.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include "agentsrv.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
		WSADATA WSAData;
		WSAStartup(0x101, &WSAData);
#endif
	AgentSrv srv("0.0.0.0:7777");
	srv.Start();
	srv.Run();
	srv.Stop();

#ifdef WIN32
	WSACleanup(); 
#endif
	return 0;
}

