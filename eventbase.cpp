
#include "eventbase.h"

#include <event2/event.h>

EventBase::EventBase()
{
	m_base = event_base_new();
	if (!m_base)
	{
		fprintf(stderr, "Couldn't create an event_base:exiting\n");
		return;
	}
}

EventBase::~EventBase()
{
	event_base_free(m_base);
}

void EventBase::Run()
{
	event_base_dispatch(m_base);
}
