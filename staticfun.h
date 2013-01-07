
#ifndef STATICFUN_H_
#define STATICFUN_H_

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <event2/util.h>

#include "agentclient.h"
#include "AgentSrv.h"

#include <assert.h>

static void inreadcb(struct bufferevent *bev, void *arg)
{
	AgentClient* client = (AgentClient*)arg;
	assert(client);

	client->ReadRequest();
}

static void eventcb(struct bufferevent *bev, short what, void *arg)
{
	AgentClient* client = (AgentClient*)arg;
	assert(client);

	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF| BEV_EVENT_TIMEOUT))
	{
		client->CloseInBufEvent();
	}
}

static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *a, int slen, void *arg)
{
	AgentSrv* srv = (AgentSrv*)arg;
	assert(srv);

	EventBase* eventbase = srv->GetEventBase();

	struct bufferevent* bufevent = bufferevent_socket_new(eventbase->GetBase(), fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);

	AgentClient* client = new AgentClient(srv, bufevent);

	bufferevent_setcb(bufevent, inreadcb, NULL, eventcb, client);
	bufferevent_enable(bufevent, EV_READ);
}

static void outreadereventcb(struct bufferevent *bev, short what, void *arg)
{
	AgentClient *client = (AgentClient* )arg;
	assert(client);

	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT))
	{
		client->CloseOutBufEvent();
		return;
	}

	if (what & BEV_EVENT_CONNECTED)
	{
		client->SendRequestHeads();
	}
}

static void outreadcb(struct bufferevent *bev, void *arg)
{
	AgentClient* client = (AgentClient*)arg;
	assert(client);

	//printf("static void outreadcb\n");
	client->ReadRespond();
}

static void exchangeInput(struct bufferevent *bev, void *arg)
{
	AgentClient* client = (AgentClient*)arg;
	assert(client);

	client->ExchangeInputData();
}

static void exchangeOutput(struct bufferevent *bev, void *arg)
{
	AgentClient* client = (AgentClient*)arg;
	assert(client);

	client->ExchangeOutputData();
}

#endif
