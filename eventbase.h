
#ifndef EVENTBASE_H_
#define EVENTBASE_H_

struct event_base;
class EventBase
{
public:
	EventBase();
	~EventBase();

	void Run();

public:
	struct event_base* GetBase()
	{
		return m_base;
	}
private:
	struct event_base* m_base;
};
#endif
