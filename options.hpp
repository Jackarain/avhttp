#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#pragma once

#include <map>

namespace avhttp {

class options
{
public:

	// http选项.
	typedef std::map<std::string, std::string> option_item;

public:
	options () {}
	~options () {}

public:

	// 添加http选项.
	void insert(const std::string &key, const std::string &val)
	{
		m_opts[key] = val;
	}

	// 删除http选项.
	void remove(const std::string &key)
	{
		option_item::iterator f = m_opts.find(key);
		if (f != m_opts.end())
			m_opts.erase(f);
	}

	// 清空http选项.
	void clear()
	{
		m_opts.clear();
	}

	// 返回所有option_item.
	option_item& option_all()
	{
		return m_opts;
	}

protected:
	option_item m_opts;
};

}

#endif // __OPTIONS_H__
