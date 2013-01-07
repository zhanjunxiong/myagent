
#ifndef HTTPMESSAGE_H_
#define HTTPMESSAGE_H_

#include <string>
#include <vector>

class MessageItem
{
public:
	MessageItem(char* name, char* value):
		m_name(name),
		m_value(value)
	{}

	MessageItem(const std::string& name, const std::string& value):
		m_name(name),
		m_value(value)
	{}

	std::string	m_name;
	std::string	m_value;
};

class HttpMessage
{
public:
	HttpMessage();
	~HttpMessage();

	const std::string GetHeader(const std::string& name);
	void SetHeader(const char* name, const char* value);
	void SetHeader(const std::string& name, const std::string& value);
	const std::string GetHeaders();
	bool RemoveHeader(const std::string& name);

	void Clear();
private:
	std::vector<MessageItem*> m_items;
};

#endif
