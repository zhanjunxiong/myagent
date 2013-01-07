
#ifndef DNSBASE_H_
#define DNSBASE_H_

#include <string>
#include <map>

#include "myconfig.h"

class EventBase;
struct evdns_base;
struct evdns_getaddrinfo_request;
class DnsBase
{
public:
	DnsBase(EventBase* eventbase);
	~DnsBase();

	struct evdns_getaddrinfo_request* GetAddrInfo(const std::string& host, int port, void* arg);

	void SetIPcache(const std::string& addr, const std::string& ip);
private:
	const std::string GetIPbyAddr(const std::string& addr);
	void RemoveIPbyAddr(const std::string& addr);
	void ParseConfig();
private:
	struct evdns_base* m_dnsbase;
	std::map<std::string, std::string> m_ipcache;
	MyConfig m_ipconfig;
};
#endif
