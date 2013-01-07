/*
 * myconfig.cpp
 *
 *  Created on: 2012-11-27
 *      Author: Administrator
 */

#include "myconfig.h"

#include <iostream>
#include <fstream>

MyConfig::MyConfig()
{
}

MyConfig::~MyConfig()
{

}

bool MyConfig::Parse(const std::string& filename)
{
	std::ifstream fin(filename.c_str());
	std::string line;
	while(!fin.eof())
	{
		std::getline(fin,line);
		//printf("line: %s\n", line.c_str());

        if(line.length()<1 || line[0]=='#')
            continue;

        size_t ippos = line.find_first_of(" ");
        if (ippos == std::string::npos)
        	continue;

        size_t startaddrpos = line.find_first_not_of(" ", ippos + 1);
        if (startaddrpos == std::string::npos)
        	continue;

        size_t endaddrpos = line.find_first_of(" ", startaddrpos + 1);
        if (endaddrpos == std::string::npos)
        	continue;

        std::string ip(line.substr(0, ippos));
        std::string addr(line.substr(startaddrpos, endaddrpos - startaddrpos));
        std::string op(line.substr(endaddrpos + 1, 1));
        printf("ip[%s], addr[%s], op[%s]\n", ip.c_str(), addr.c_str(), op.c_str());
        struct ipaddr ipaddr;
        ipaddr.ip = ip;
        ipaddr.op = op;
        SetIPcache(addr, ipaddr);
	}
	fin.close();
	return true;
}

const struct ipaddr MyConfig::GetIPbyAddr(const std::string& addr)
{
    if(m_cache.find(addr) != m_cache.end())
    {
      	return m_cache[addr];
    }

    static struct ipaddr sipaddr;
    return sipaddr;
}

void MyConfig::SetIPcache(const std::string& addr, const struct ipaddr& ipaddr)
{
	m_cache[addr] = ipaddr;
}

void MyConfig::RemoveIPbyAddr(const std::string& addr)
{
	m_cache.erase(addr);
}
