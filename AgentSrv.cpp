
#include "AgentSrv.h"
#include "agentclient.h"
#include "dnsbase.h"
#include "eventbase.h"

#include <assert.h>
#include <signal.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "staticfun.h"
///////////////////////////////////////////////////////////
AgentSrv::AgentSrv(const char* address):
	m_addr(address)
{
	m_base = new EventBase();
	m_dnsbase = new DnsBase(m_base);
}

AgentSrv::~AgentSrv()
{
	delete m_dnsbase;
	delete m_base;
}

bool AgentSrv::Start()
{
#ifdef WIN32
		WSADATA WSAData;
		WSAStartup(0x101, &WSAData);
#else
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			return false;
#endif

	struct event_base* base = m_base->GetBase();
	if (!base)
	{
		return false;
	}

	struct sockaddr_storage listen_on_addr;
	memset(&listen_on_addr, 0, sizeof(listen_on_addr));
	int socklen = sizeof(listen_on_addr);
	if (evutil_parse_sockaddr_port(m_addr.c_str(), (struct sockaddr*)&listen_on_addr, &socklen)<0)
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)&listen_on_addr;
		sin->sin_port = 7777;
		sin->sin_addr.s_addr = htonl(0x7f000001); // localhost
		sin->sin_family = AF_INET;
		socklen = sizeof(struct sockaddr_in);
	}
	m_listener = evconnlistener_new_bind(base, accept_cb, this,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
	    -1, (struct sockaddr*)&listen_on_addr, socklen);

	return true;
}

void AgentSrv::Run()
{
	m_base->Run();
}

void AgentSrv::Stop()
{
	evconnlistener_free(m_listener);
}
