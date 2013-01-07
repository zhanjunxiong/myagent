
#ifndef HTTPRESPOND_H_
#define HTTPRESPOND_H_

#include "httpmessage.h"
#include <string>


class HttpRespond
{
public:
	HttpRespond();
	~HttpRespond();

	void Clear();
public:
	std::string m_version;
	std::string m_code;
	std::string m_reason;
	HttpMessage m_heads;
	int m_bodylens;
	bool m_ischucks;
	int m_chucklen;
};

#endif
