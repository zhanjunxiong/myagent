
#include "httprespond.h"
HttpRespond::HttpRespond():
	m_bodylens(0),
	m_ischucks(false),
	m_chucklen(0)
{
}

HttpRespond::~HttpRespond()
{
}


void HttpRespond::Clear()
{
	m_ischucks = false;
	m_chucklen = 0;
	m_bodylens = 0;
	m_heads.Clear();
}

