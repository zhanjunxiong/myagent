
#ifndef AGENTCLIENT_H_
#define AGENTCLIENT_H_

#include "httprequest.h"
#include "httprespond.h"

#include <event2/util.h>

enum request_parse_state
{
	REQUEST_PARSE_READING_FIRSTLINE,
	REQUEST_PARSE_READING_HEADERS,
	REQUEST_PARSE_READING_BODY,
};

enum respond_parse_state
{
	RESPOND_PARSE_READING_FIRSTLINE,
	RESPOND_PARSE_READING_HEADERS,
	RESPOND_PARSE_READING_BODY,

	RESPOND_PARSE_READING_CHUCK_SIZE,
	RESPOND_PARSE_READING_CHUCK_BODY,
	RESPOND_PARSE_READING_CHUCK_CRLF,
	RESPOND_PARSE_READING_CHUCK_TAIL,
};

struct evdns_getaddrinfo_request;
class AgentSrv;
class AgentClient
{
public:
	AgentClient(AgentSrv* srv, struct bufferevent* inbufevent);
	~AgentClient();

	void ConnectAddr(struct evutil_addrinfo *addr);
	void ConnectAddr(struct sockaddr *sockaddr, int len);

	void CloseInBufEvent();
	void CloseOutBufEvent();

public:
	void SendRequest();

	void ReadRequest();
	void ReadRespond();

	bool SendRequestHeads();

	void RedirectUrl();
public:
	void CancelReq();

private:
	bool StartSendRequest();
private:
	void Destroy();
private:
	// request
	bool ReadRequestFirstline();
	bool ReadRequestHeaders();
	bool ReadRequestBody();

private:
	// respond
	bool ReadRespondFirstline();
	bool ReadRespondHeaders();
	bool ReadRespondBody();
	bool ReadRespondChuckBody();
	bool ReadRespondChuckSize();
	bool ReadRespondChuckCRLF();
	bool ReadRespondChuckTail();

	bool SendRespondHeads();
public:
	bool SendConnectRequest();
	void ExchangeInputData();
	void ExchangeOutputData();
private:
	// util
	bool IsMethod(const std::string& method);
	bool ParseHost(const std::string& uri);
	bool ParseUri(const std::string& uri);

	void ClearAllOutBufData();
public:
	int m_id;
	struct bufferevent* m_inbufevent;
	struct bufferevent* m_outbufevent;
	AgentSrv* m_srv;
	struct evdns_getaddrinfo_request * m_req;

	enum request_parse_state m_request_state;
	enum respond_parse_state m_respond_state;
	HttpRequest m_httpRequest;
	HttpRespond m_httpRespond;
};

#endif
