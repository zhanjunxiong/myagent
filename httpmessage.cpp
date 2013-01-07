
#include "httpmessage.h"

#include <sstream>
#include "string.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif

HttpMessage::HttpMessage()
{

}

HttpMessage::~HttpMessage()
{
	Clear();
}

const std::string HttpMessage::GetHeader(const std::string& name)
{
	size_t len = m_items.size();
	for (size_t i = 0; i < len; i++)
	{
		MessageItem* item = m_items[i];
		if (strcasecmp(name.c_str(), item->m_name.c_str()) == 0)
		{
			return item->m_value;
		}
	}
	return "";
}

void HttpMessage::SetHeader(const char* name, const char* value)
{
	//printf("setheader name:[%s], value[%s]\n", name, value);
	const char* tname = name;
	if (strcasecmp(name, "Proxy-Connection") == 0)
	{
		//SetHeader("Connection", "close");
		//return;
		tname = "Connection";
	}

	size_t len = m_items.size();
	for(size_t i=0; i<len; i++)
	{
		MessageItem* item = m_items[i];
		if (strcasecmp(tname, item->m_name.c_str()) == 0)
		{
			item->m_value = value;
			return;
		}
	}
	MessageItem* item = new MessageItem(tname, value);
	m_items.push_back(item);
}

void HttpMessage::SetHeader(const std::string& name, const std::string& value)
{
	SetHeader(name.c_str(), value.c_str());
}

const std::string HttpMessage::GetHeaders()
{
	std::stringstream buf;
	size_t len = m_items.size();
	for(size_t i=0; i< len; i++)
	{
		MessageItem* item = m_items[i];
		if (item)
		{
			buf << item->m_name;
			buf << ": ";
			buf << item->m_value;
			buf << "\r\n";
		}
	}
	return buf.str();
}

bool HttpMessage::RemoveHeader(const std::string& name)
{
	size_t len = m_items.size();
    for(size_t i=0; i< len; i++)
    {
    	MessageItem* item = m_items[i];
		if(strcasecmp(name.c_str(), item->m_name.c_str()) == 0)
		{
			m_items.erase(m_items.begin() + i);
			delete item;
			return true;
		}
    }
	return false;
}

void HttpMessage::Clear()
{
	size_t len = m_items.size();
	for (size_t i = 0; i < len; i++)
	{
		MessageItem* item = m_items[i];
		delete item;
	}
	m_items.clear();
}
