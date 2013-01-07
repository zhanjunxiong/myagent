
#include "dnsbase.h"

#include "agentclient.h"
#include "config.h"
#include "eventbase.h"

#include <event2/dns.h>
#include <event2/util.h>

#include <assert.h>

#ifdef WIN32
#define snprintf _snprintf_s
#endif

static void dnscallback(int errcode, struct evutil_addrinfo *addr, void *ptr)
{
    if (errcode)
    {
        printf("%s\n", evutil_gai_strerror(errcode));
        return;
    }

	AgentClient *client = (AgentClient* )ptr;
	assert(client);

	client->ConnectAddr(addr);
    return;
}

//////////////////////////////////////////////////////////////////////////
DnsBase::DnsBase(EventBase* eventbase)
{
	struct event_base* base = eventbase->GetBase();
	m_dnsbase = evdns_base_new(base, 1);
	if (!m_dnsbase)
	{
		fprintf(stderr, "Couldn't create an evdns_base:exiting.\n");
		return;
	}
	m_ipcache.clear();
	ParseConfig();
}

DnsBase::~DnsBase()
{
	evdns_base_free(m_dnsbase, 0);
	m_ipcache.clear();
}

struct evdns_getaddrinfo_request* DnsBase::GetAddrInfo(const std::string& host, int port, void* arg)
{
	AgentClient* client = (AgentClient*)arg;

	std::string ip;
	struct ipaddr ipaddr = m_ipconfig.GetIPbyAddr(host);
	if (!ipaddr.ip.empty())
	{
		printf("From ipaddr\n\n");
		ip = ipaddr.ip;
		if (ipaddr.op == "s" && client->m_httpRequest.m_method != "CONNECT")
		{
			client->RedirectUrl();
		}
	}

	if (ip.empty())
	{
		ip = GetIPbyAddr(host);
	}
	if (!ip.empty())
	{
		printf("DnsBase::GetAddrInfo cache, host[%s:%d], ip[%s]===============\n", host.c_str(), port, ip.c_str());
		char tmp[128];
		snprintf(tmp, 127, "%s:%d", ip.c_str(), port);
		struct sockaddr_storage addr;
		memset(&addr, 0, sizeof(addr));
		int socklen = sizeof(addr);
		if (evutil_parse_sockaddr_port(tmp, (struct sockaddr*)&addr, &socklen)<0)
		{
			printf("DnsBase::GetAddrInfo error\n");
			return NULL;
		}

		client->ConnectAddr((struct sockaddr*)&addr, socklen);
		return NULL;
	}

	struct evutil_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = EVUTIL_AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char sport[10];
    snprintf(sport, 9, "%d", port);
    printf("GetAddrInfo, host:%s, port:%s\n", host.c_str(), sport);
	evdns_getaddrinfo_request* req = evdns_getaddrinfo(m_dnsbase, host.c_str(), sport, &hints, dnscallback, arg);
    if (req == NULL)
    {
    	printf("[request for %s returned immediately]\n", host.c_str());
    }
    return req;
}

const std::string DnsBase::GetIPbyAddr(const std::string& addr)
{
    if(m_ipcache.find(addr) != m_ipcache.end())
    {
      	return m_ipcache[addr];
    }
    return "";
}

void DnsBase::SetIPcache(const std::string& addr, const std::string& ip)
{
	m_ipcache[addr] = ip;
}

void DnsBase::RemoveIPbyAddr(const std::string& addr)
{
	m_ipcache.erase(addr);
}


#include <vector>
#include <sstream>
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	//printf("%s\n", s.c_str());
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
    	//printf("item:%s\n", item.c_str());
        elems.push_back(item);
    }
    return elems;
}

void DnsBase::ParseConfig()
{
    printf("Loading config ...\n");
    std::vector<std::string> lines = split(Config::defaults, '\n');
    printf("lines size:%d\n", lines.size());
    for(size_t i=0; i<lines.size(); i++)
    {
        std::string line = lines[i];
        if(line.length()<1 || line[0]=='#')
            continue;
        //printf("line[%s]\n", line.c_str());
        size_t ippos = line.find(" ");
        if (ippos == std::string::npos)
        	continue;

        std::string ip = line.substr(0, ippos);
       // printf("ip[%s]\n", ip.c_str());
        std::string addr = line.substr(ippos + 1);
        printf("ip[%s], addr[%s]\n", ip.c_str(), addr.c_str());
        SetIPcache(addr, ip);
    }

   printf("loading config end...\n");

   printf("start read host conf..\n");
   m_ipconfig.Parse("host.conf");
   printf("end read host conf..\n");
}
