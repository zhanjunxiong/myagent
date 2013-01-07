
#include "agentclient.h"

#include "AgentSrv.h"
#include "dnsbase.h"
#include "httpmessage.h"

#include "staticfun.h"

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <event2/util.h>

#include <assert.h>

#include "string.h"

#ifdef WIN32
#define strcasecmp _stricmp
#define	strncasecmp	_strnicmp
#endif
///////////////////////////////////////////////////////////////////////////////////////
static int agentclientid = 1;
AgentClient::AgentClient(AgentSrv* srv, struct bufferevent* inbufevent):
	m_id(agentclientid++),
	m_inbufevent(inbufevent),
	m_outbufevent(NULL),
	m_srv(srv),
	m_req(NULL),
	m_request_state(REQUEST_PARSE_READING_FIRSTLINE),
	m_respond_state(RESPOND_PARSE_READING_FIRSTLINE)
{
}

AgentClient::~AgentClient()
{
	//CancelReq();
}

bool AgentClient::StartSendRequest()
{
	if (m_outbufevent)
	{
		printf("AgentClient::StartSendRequest double time, client[%d]\n", m_id);
		SendRequestHeads();
		return true;
	}

	bufferevent_disable(m_inbufevent, EV_READ);
    DnsBase* dnsbase = m_srv->GetDnsBase();
    m_req = dnsbase->GetAddrInfo(m_httpRequest.m_host, m_httpRequest.m_port, this);
    return true;
}

void AgentClient::ConnectAddr(struct evutil_addrinfo *addr)
{
	if (!m_srv) return;
	EventBase* eventbase = m_srv->GetEventBase();
	if (!eventbase) return;

	struct event_base* base = eventbase->GetBase();

    int flags=BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS;
    m_outbufevent = bufferevent_socket_new(base, -1, flags);

	bufferevent_setcb(m_outbufevent, outreadcb,NULL, outreadereventcb, this);
	bufferevent_enable(m_outbufevent, EV_READ);

	struct evutil_addrinfo *curai = addr;
	bool b = true;
	while(curai)
	{
		if (bufferevent_socket_connect(m_outbufevent, curai->ai_addr, curai->ai_addrlen)<0)
		{
			perror("bufferevent_socket_connect");
			struct evutil_addrinfo* next = curai->ai_next;
			curai = next;
		}
		else
		{
			b = false;
			break;
		}
	}

	if (b)
	{
		CloseInBufEvent();
	}
	else
	{
		DnsBase* dnsbase = m_srv->GetDnsBase();
		if (!dnsbase) return;
		if (!curai) return;

		char buf[128];
		struct sockaddr_in *sin = (struct sockaddr_in *)curai->ai_addr;
		evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);

		printf("host[%s], ip[%s]\n", m_httpRequest.m_host.c_str(), buf);
		dnsbase->SetIPcache(m_httpRequest.m_host, std::string(buf));
	}

	m_req = NULL;
	evutil_freeaddrinfo(addr);
}

void AgentClient::ConnectAddr(struct sockaddr *sockaddr, int len)
{
	//printf("========== ConnectAddr, client[%d]\n", m_id);
	if (!m_srv) return;
	EventBase* eventbase = m_srv->GetEventBase();
	if (!eventbase) return;

	struct event_base* base = eventbase->GetBase();

    int flags=BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS;
    m_outbufevent = bufferevent_socket_new(base, -1, flags);

	bufferevent_setcb(m_outbufevent, outreadcb,NULL, outreadereventcb, this);
	bufferevent_enable(m_outbufevent, EV_READ);

	if (bufferevent_socket_connect(m_outbufevent, sockaddr, len)<0)
	{
		perror("bufferevent_socket_connect");
	}
}

void AgentClient::ReadRequest()
{
	if (!m_inbufevent) return;

	switch(m_request_state)
	{
	case REQUEST_PARSE_READING_FIRSTLINE:
		ReadRequestFirstline();
		break;
	case REQUEST_PARSE_READING_HEADERS:
		ReadRequestHeaders();
		break;
	case REQUEST_PARSE_READING_BODY:
		ReadRequestBody();
		break;
	}
}

void AgentClient::CloseInBufEvent()
{
	//printf("AgentClient::CloseInBufEvent, clientid[%d]\n", m_id);
	if (m_inbufevent)
	{
		bufferevent_free(m_inbufevent);
		m_inbufevent = NULL;
	}

	if (m_outbufevent)
	{
		//evutil_socket_t fd = bufferevent_getfd(m_outbufevent);
		//evutil_closesocket(fd);
		bufferevent_free(m_outbufevent);
		m_outbufevent = NULL;
		return;
	}

	Destroy();
}

void AgentClient::CloseOutBufEvent()
{
	//printf("AgentClient::CloseOutBufEvent, clientid[%d]\n", m_id);
	ClearAllOutBufData();
	if (m_outbufevent)
	{
		bufferevent_free(m_outbufevent);
		m_outbufevent = NULL;
	}

	if (m_inbufevent)
	{
		//evutil_socket_t fd = bufferevent_getfd(m_inbufevent);
		//evutil_closesocket(fd);
		bufferevent_free(m_inbufevent);
		m_inbufevent = NULL;
		return;
	}

	Destroy();
}

void AgentClient::ClearAllOutBufData()
{
	if (!m_outbufevent || !m_inbufevent) return;

	bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);
	evbuffer_add_buffer(dst, src);
}

void AgentClient::Destroy()
{
	if (m_req)
	{
		evdns_getaddrinfo_cancel(m_req);
	}
	delete this;
}

void AgentClient::ReadRespond()
{
	if (!m_outbufevent || !m_inbufevent) return;

	//printf("+++++ client[%d], state[%d]\n", m_id, m_respond_state);
	switch(m_respond_state)
	{
	case RESPOND_PARSE_READING_FIRSTLINE:
		ReadRespondFirstline();
		break;
	case RESPOND_PARSE_READING_HEADERS:
		ReadRespondHeaders();
		break;
	case RESPOND_PARSE_READING_BODY:
		ReadRespondBody();
		break;
	case RESPOND_PARSE_READING_CHUCK_BODY:
		ReadRespondChuckBody();
		break;
	case RESPOND_PARSE_READING_CHUCK_SIZE:
		ReadRespondChuckSize();
		break;
	case RESPOND_PARSE_READING_CHUCK_CRLF:
		ReadRespondChuckCRLF();
		break;
	case RESPOND_PARSE_READING_CHUCK_TAIL:
		ReadRespondChuckTail();
		break;
	}
}

void AgentClient::SendRequest()
{
	if (!m_outbufevent || !m_inbufevent)
	{
		return;
	}
	SendRequestHeads();
}

void AgentClient::CancelReq()
{
	if (m_req)
	{
		evdns_getaddrinfo_cancel(m_req);
		m_req = NULL;
	}
}

bool AgentClient::ReadRequestFirstline()
{
	m_httpRequest.Clear();
	struct evbuffer* buf = bufferevent_get_input(m_inbufevent);
	size_t line_length;
	char* line = evbuffer_readln(buf, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) return false;
	printf("client[%d], ReadRequestFirstline, line[%s]\n", m_id, line);
	std::string firstline(line);
	free(line);

	size_t pos = 0;
	size_t methodpos = firstline.find_first_of(" ", pos);
	if(methodpos == std::string::npos)
	{
		return false;
	}

	std::string method = firstline.substr(pos, methodpos - pos);
	if (!IsMethod(method))
	{
		return false;
	}

	size_t uripos = firstline.find_first_of(" ", methodpos + 1);
	if (uripos == std::string::npos)
	{
		return false;
	}
	std::string uri = firstline.substr(methodpos + 1, uripos - methodpos - 1);

	std::string version = firstline.substr(uripos + 1);

	m_httpRequest.m_method = method;
	m_httpRequest.m_version = version;
	printf("method[%s], version[%s]\n", method.c_str(), version.c_str());
	if(!ParseUri(uri))
	{
		return false;
	}

	m_request_state = REQUEST_PARSE_READING_HEADERS;
	ReadRequestHeaders();
	return true;
}

bool AgentClient::ReadRequestHeaders()
{
	struct evbuffer* buf = bufferevent_get_input(m_inbufevent);
	size_t line_length;
	char* line;
	bool flag = false;
	while ((line = evbuffer_readln(buf, &line_length, EVBUFFER_EOL_CRLF)) != NULL)
	{
		std::string sline(line);
		free(line);

		if (sline.empty())
		{
			flag = true;
			break;
		}

		size_t keypos = sline.find_first_of(":", 0);
		if (keypos == std::string::npos)
		{
			return false;
		}

		std::string skey(sline.substr(0, keypos));
		std::string svalue(sline.substr(keypos + 2));

		m_httpRequest.m_heads.SetHeader(skey, svalue);
	}

	if (flag)
	{
		std::string slen = m_httpRequest.m_heads.GetHeader("content-length");
		m_httpRequest.m_bodylens = atoi(slen.c_str());
		StartSendRequest();
		if (m_httpRequest.m_bodylens > 0)
		{
			m_request_state = REQUEST_PARSE_READING_BODY;
			ReadRequestBody();
		}
		else
		{
			m_request_state = REQUEST_PARSE_READING_FIRSTLINE;
			//ReadRequestFirstline();
		}
	}
	return false;
}

bool AgentClient::ReadRequestBody()
{
	if (!m_outbufevent) return false;

	if (m_httpRequest.m_bodylens <= 0)
	{
		m_request_state = REQUEST_PARSE_READING_FIRSTLINE;
		//ReadRequestFirstline();
		return true;
	}

	//
	bufferevent_enable(m_outbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_inbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_outbufevent);
	m_httpRequest.m_bodylens -= evbuffer_remove_buffer(src, dst, m_httpRequest.m_bodylens);
	if (m_httpRequest.m_bodylens <= 0)
	{
		m_request_state = REQUEST_PARSE_READING_FIRSTLINE;
		//ReadRequestFirstline();
		return true;
	}
	return true;
}

bool AgentClient::SendRequestHeads()
{
	if (!m_outbufevent || !m_inbufevent) return false;
	if (m_httpRequest.m_method == "CONNECT")
	{
		SendConnectRequest();
		return true;
	}
	bufferevent_enable(m_outbufevent, EV_WRITE);
	struct evbuffer* buf = bufferevent_get_output(m_outbufevent);
	if (m_httpRequest.m_path == "")
	{
		evbuffer_add_printf(buf, "%s %s\r\n", m_httpRequest.m_method.c_str(), m_httpRequest.m_version.c_str());
	}
	else
	{
		evbuffer_add_printf(buf, "%s %s %s\r\n", m_httpRequest.m_method.c_str(), m_httpRequest.m_path.c_str(), m_httpRequest.m_version.c_str());
	}
	evbuffer_add_printf(buf, "%s\r\n", m_httpRequest.m_heads.GetHeaders().c_str());

	printf("client[%d]%s %s %s\r\n", m_id, m_httpRequest.m_method.c_str(), m_httpRequest.m_path.c_str(), m_httpRequest.m_version.c_str());
	//printf("%s\r\n", m_httpRequest.m_heads.GetHeaders().c_str());

	return true;
}

void AgentClient::RedirectUrl()
{
	if (!m_inbufevent) return;

	std::string redirectUrl = "https://" + m_httpRequest.m_host + m_httpRequest.m_path;
	struct evbuffer* inbuf = bufferevent_get_output(m_inbufevent);
	evbuffer_add_printf(inbuf, "HTTP/1.1 302 Found\r\n");
	evbuffer_add_printf(inbuf, "Location: %s\r\n\r\n", redirectUrl.c_str());
	printf("client[%d], redirecturl[%s]\n", m_id, redirectUrl.c_str());
}

bool AgentClient::SendConnectRequest()
{
	if (!m_outbufevent || !m_inbufevent) return false;
	//printf("client[%d], AgentClient::SendConnectRequest connect!!\n", m_id);
	//if (strcasecmp(m_httpRequest.m_method.c_str(), "CONNECT") == 0)
	{
		//bufferevent_enable(m_inbufevent, EV_WRITE);
		struct evbuffer* inbuf = bufferevent_get_output(m_inbufevent);
		evbuffer_add_printf(inbuf, "HTTP/1.1 200 OK\r\n\r\n");

		bufferevent_setcb(m_inbufevent, exchangeInput, NULL, eventcb, this);
		bufferevent_setcb(m_outbufevent, exchangeOutput, NULL, outreadereventcb, this);
		bufferevent_enable(m_inbufevent, EV_READ | EV_WRITE);
		bufferevent_enable(m_outbufevent, EV_READ | EV_WRITE);
	}
	return true;
}

bool AgentClient::IsMethod(const std::string& method)
{
	//printf("client[%d], method[%s]\n", m_id, method.c_str());
	if (evutil_ascii_strcasecmp(method.c_str(), "get") == 0)
	{
		return true;
	}

	if (evutil_ascii_strcasecmp(method.c_str(), "post") == 0)
	{
		return true;
	}

	if (evutil_ascii_strcasecmp(method.c_str(), "connect") == 0)
	{
		return true;
	}

	if (evutil_ascii_strcasecmp(method.c_str(), "head") == 0)
	{
		return true;
	}
	return false;
}

bool AgentClient::ParseHost(const std::string& host)
{
    size_t hostEnd = host.find(':');
	if(hostEnd == std::string::npos)
	{
		m_httpRequest.m_host = host;
		m_httpRequest.m_port = 80;
	}
	else
	{
		std::string port(host.substr(hostEnd + 1, host.length() - hostEnd - 1));
		m_httpRequest.m_port = atoi(port.c_str());
		m_httpRequest.m_host =host.substr(0, hostEnd);
	}
	return true;
}

bool AgentClient::ParseUri(const std::string& uri)
{
	if (strncasecmp(uri.c_str(), "http://", 7) == 0)
	{
		size_t hostend = uri.find_first_of('/', 8);
		if (hostend == std::string::npos)
		{
			return false;
		}

		std::string host(uri.substr(7, hostend - 7));
		if(!ParseHost(host))
		{
			return false;
		}
		m_httpRequest.m_path = uri.substr(hostend, uri.length() - hostend);
		return true;
	}

	if (evutil_ascii_strcasecmp(m_httpRequest.m_method.c_str(), "CONNECT") == 0)
	{
		/*
		std::string host(uri);
		size_t hostend = uri.find_first_of('/', 8);
		if (hostend != std::string::npos)
		{
			host = uri.substr(0, hostend);
			m_httpRequest.m_path=  uri.substr(hostend, uri.length() - hostend);
		}
		else
		{
			m_httpRequest.m_path = "/";
		}
		*/
		if (!ParseHost(uri))
		{
			return false;
		}
		m_httpRequest.m_path ="";
		return true;
	}

	return false;
}

bool AgentClient::ReadRespondFirstline()
{
	m_httpRespond.Clear();
	struct evbuffer* buf = bufferevent_get_input(m_outbufevent);
	size_t line_length;
	char* line = evbuffer_readln(buf, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) return false;
	printf("client[%d], respond cmd[%s]\n", m_id, line);
	std::string firstline(line);
	free(line);

	size_t pos = 0;
	size_t versionpos = firstline.find_first_of(" ", pos);
	if(versionpos == std::string::npos)
	{
		printf("client[%d], versionpos is null\n", m_id);
		return false;
	}
	std::string version = firstline.substr(pos, versionpos - pos);
	size_t httppos = version.find_first_of("/");
	if (httppos == std::string::npos) return false;
	std::string http = version.substr(0, httppos);
	if (evutil_ascii_strcasecmp(http.c_str(), "http") != 0)
	{
		printf("client[%d], respond, http[%s]\n", m_id, http.c_str());
		return false;
	}
	size_t codepos = firstline.find_first_of(" ", versionpos + 1);
	if (codepos == std::string::npos)
	{
		printf("client[%d], codepos is null\n", m_id);
		return false;
	}
	std::string code = firstline.substr(versionpos + 1, codepos - versionpos - 1);
	std::string reason = firstline.substr(codepos + 1);

	m_httpRespond.m_version = version;
	m_httpRespond.m_code = code;
	m_httpRespond.m_reason = reason;

	m_respond_state = RESPOND_PARSE_READING_HEADERS;
	ReadRespondHeaders();
	return true;
}

bool AgentClient::ReadRespondHeaders()
{
	struct evbuffer* buf = bufferevent_get_input(m_outbufevent);

	size_t line_length;
	char* line;
	bool flag = false;
	while ((line = evbuffer_readln(buf, &line_length, EVBUFFER_EOL_CRLF)) != NULL)
	{
		std::string sline(line);
		free(line);
		//printf("client[%d] heads:[%s]\n", m_id, sline.c_str());
		if ( sline.empty())
		{
			flag = true;
			break;
		}
		size_t keypos = sline.find_first_of(":", 0);
		if (keypos == std::string::npos)
		{
			return false;
		}

		std::string skey(sline.substr(0, keypos));
		std::string svalue(sline.substr(keypos + 2));

		m_httpRespond.m_heads.SetHeader(skey, svalue);
	}

	if (flag)
	{
		SendRespondHeads();

		std::string encoding = m_httpRespond.m_heads.GetHeader("Transfer-Encoding");
		if ( evutil_ascii_strcasecmp(encoding.c_str(), "chunked") == 0)
		{
			m_respond_state = RESPOND_PARSE_READING_CHUCK_SIZE;
			m_httpRespond.m_ischucks = true;
			ReadRespondChuckSize();
			return true;
		}
		else
		{
			m_respond_state = RESPOND_PARSE_READING_BODY;
			std::string slen = m_httpRespond.m_heads.GetHeader("content-length");
			int len = atoi(slen.c_str());
			m_httpRespond.m_bodylens = len;
			ReadRespondBody();
		}
	}
	return false;
}

bool AgentClient::ReadRespondBody()
{
	//printf("AgentClient::ReadRespondBody,  clientid[%d] len [%d]\n", m_id, m_httpRespond.m_bodylens);
	if (m_httpRespond.m_bodylens <= 0)
	{
		bufferevent_enable(m_inbufevent, EV_WRITE);
		m_httpRespond.m_bodylens = 0;
		m_respond_state = RESPOND_PARSE_READING_FIRSTLINE;
		ReadRespondFirstline();
		return true;
	}
	//

	//bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);
//	if (evbuffer_get_length(src) < m_httpRespond.m_bodylens)
	//{
		//printf("===========+++==clientid[%d] src[%d], bodylens[%d]\n", m_id, evbuffer_get_length(src), m_httpRespond.m_bodylens);
		//return false;
	//}

	int readlen = evbuffer_remove_buffer(src, dst, m_httpRespond.m_bodylens);
	m_httpRespond.m_bodylens -= readlen;
	//printf("AgentClient::ReadRespondBody, clientid[%d]readlen[%d] len [%d]\n", m_id, readlen, m_httpRespond.m_bodylens);

	if (m_httpRespond.m_bodylens <= 0)
	{
		bufferevent_enable(m_inbufevent, EV_WRITE);
		//m_httpRespond.m_bodylens = 0;
		m_respond_state = RESPOND_PARSE_READING_FIRSTLINE;
		ReadRespondFirstline();
		return true;
	}
	bufferevent_enable(m_outbufevent, EV_READ);
	return true;
}

bool AgentClient::ReadRespondChuckBody()
{
	//bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);
	m_httpRespond.m_chucklen -= evbuffer_remove_buffer(src, dst, m_httpRespond.m_chucklen);
	//printf("---------AgentClient::ReadRespondChuckBody, clientid[%d] left chucklen:%d\n", m_id, m_httpRespond.m_chucklen);
	if (m_httpRespond.m_chucklen <= 0)
	{
		m_httpRespond.m_chucklen = 0;
		m_respond_state = RESPOND_PARSE_READING_CHUCK_CRLF;
		ReadRespondChuckCRLF();
	}

	return true;
}

bool AgentClient::ReadRespondChuckCRLF()
{
	//bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);

	size_t line_length;
	char* line = evbuffer_readln(src, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) return false;

	evbuffer_add_printf(dst, "%s\r\n", line);
	free(line);

	m_respond_state = RESPOND_PARSE_READING_CHUCK_SIZE;
	ReadRespondChuckSize();
	return true;
}

bool AgentClient::ReadRespondChuckTail()
{
	if (!m_outbufevent || !m_inbufevent) return false;

	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);

	size_t line_length;
	char* line;
	while ((line = evbuffer_readln(src, &line_length, EVBUFFER_EOL_CRLF)) != NULL)
	{
		evbuffer_add_printf(dst, "%s\r\n", line);
		printf("AgentClient::ReadRespondChuckTail, client[%d] line[%s]\n", m_id,  line);
		if (*line == '\0')
		{
			m_respond_state = RESPOND_PARSE_READING_FIRSTLINE;
			ReadRespondFirstline();
		}
		free(line);
	}
	return true;
}

bool AgentClient::ReadRespondChuckSize()
{
	bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);

	size_t line_length;
	char* line = evbuffer_readln(src, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) return false;

	evbuffer_add_printf(dst, "%s\r\n", line);
	int chucklen = 0;
	char* top;
	chucklen = m_httpRespond.m_chucklen = strtol(line, &top, 16);
	printf("AgentClient::ReadRespondChuckSize, client[%d] chucklen[%d], line[%s]\n", m_id, chucklen, line);
	free(line);

	if (chucklen > 0 )
	{
		m_respond_state = RESPOND_PARSE_READING_CHUCK_BODY;
		ReadRespondChuckBody();
	}
	else
	{
		m_respond_state = RESPOND_PARSE_READING_CHUCK_TAIL;
		ReadRespondChuckTail();
	}
	return true;
}

bool AgentClient::SendRespondHeads()
{
	if (!m_inbufevent) return false;

	//bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* buf = bufferevent_get_output(m_inbufevent);
	evbuffer_add_printf(buf, "%s %s %s\r\n", m_httpRespond.m_version.c_str(), m_httpRespond.m_code.c_str(), m_httpRespond.m_reason.c_str());
	// head
	evbuffer_add_printf(buf, "%s\r\n", m_httpRespond.m_heads.GetHeaders().c_str());

	return true;
}

void AgentClient::ExchangeInputData()
{
	if (!m_outbufevent || !m_inbufevent) return;

	printf("client[%d], AgentClient::ExchangeInputData\n", m_id);
	bufferevent_enable(m_outbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_inbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_outbufevent);

	evbuffer_add_buffer(dst, src);
}

void AgentClient::ExchangeOutputData()
{
	if (!m_outbufevent || !m_inbufevent) return;

	printf("client[%d], AgentClient::ExchangeOutputData\n", m_id);

	bufferevent_enable(m_inbufevent, EV_WRITE);
	struct evbuffer* src = bufferevent_get_input(m_outbufevent);
	struct evbuffer* dst = bufferevent_get_output(m_inbufevent);

	evbuffer_add_buffer(dst, src);
}
