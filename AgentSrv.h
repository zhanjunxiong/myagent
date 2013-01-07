
#ifndef AGENTSRV_H_
#define AGENTSRV_H_

#include <string>

#include "dnsbase.h"
#include "eventbase.h"

struct evconnlistener;
class AgentSrv
{
public:
	AgentSrv(const char* address);
	~AgentSrv();

	bool Start();
	void Run();
	void Stop();

	DnsBase* GetDnsBase()
	{
		return m_dnsbase;
	}
	EventBase* GetEventBase()
	{
		return m_base;
	}
private:
	std::string m_addr;
	EventBase* m_base;
	DnsBase* m_dnsbase;
	struct evconnlistener * m_listener;
};

#endif
