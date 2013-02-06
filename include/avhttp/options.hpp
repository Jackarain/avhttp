//
// options.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2011 Jack (jack.wgm@gmail.com)
//

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#pragma once

#include <map>

namespace avhttp {

// 访问http选项类. 该设置可能影响http访问流程, 主要用于定义Http Header.
// 由key/value的方式设定. 以下是特定的选项:
// "request" {"GET" | "POST" | "HEAD"} default is "GET"
class option_set
{
public:

	// http选项.
	typedef std::map<std::string, std::string> option_item;

public:
	option_set () {}
	~option_set () {}

public:

	// 添加http选项, 由key/value形式添加.
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
