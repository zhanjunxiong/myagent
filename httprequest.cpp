
#include "httprequest.h"

#include <event2/buffer.h>

HttpRequest::HttpRequest():
	m_bodylens(0)
{
}

HttpRequest::~HttpRequest()
{
}


void HttpRequest::Clear()
{
	m_bodylens = 0;
	m_heads.Clear();
}
