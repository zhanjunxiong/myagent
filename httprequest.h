
#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

#include <string>
#include "httpmessage.h"

struct evbuffer;
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();

	void Clear();
public:
	std::string m_method; // 请求方法
	std::string m_host; // 请求的域名或者ip
	unsigned short m_port;
	std::string m_path; // 请求的路�?
	std::string m_version; // 请求的版本号
	HttpMessage m_heads; //请求消息�?
	int m_bodylens; // 消息体的长度
};

#endif
