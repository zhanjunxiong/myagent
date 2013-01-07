/*
 * myconfig.h
 *
 *  Created on: 2012-11-27
 *      Author: Administrator
 */

#ifndef MYCONFIG_H_
#define MYCONFIG_H_

#include <string>
#include <map>

struct ipaddr
{
	std::string ip;
	std::string op;
};

class MyConfig
{
public:
	MyConfig();
	~MyConfig();

	const struct ipaddr GetIPbyAddr(const std::string& addr);
	void SetIPcache(const std::string& addr, const struct ipaddr& ipaddr);
	void RemoveIPbyAddr(const std::string& addr);

	bool Parse(const std::string& filename);
private:
	std::map<std::string, struct ipaddr> m_cache;
};

#endif /* MYCONFIG_H_ */
