//
// options.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __OPTIONS_HPP__
#define __OPTIONS_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <map>
#include <string>
#include <boost/algorithm/string.hpp>

namespace avhttp {

// 如果没有定义最大重定向次数, 则默认为5次最大重定向.
#ifndef AVHTTP_MAX_REDIRECTS
#define AVHTTP_MAX_REDIRECTS 5
#endif

namespace httpoptions {

	// 定义一些常用的　http 选项为 const string , 这样就不用记忆那些单词了，呵呵.
	static const std::string request_method("_request_method");
	static const std::string request_body("_request_body");
	static const std::string status_code("_status_code");
	static const std::string cookie("cookie");
	static const std::string referer("Referer");
	static const std::string content_type("Content-Type");
	static const std::string content_length("Content-Length");
	static const std::string connection("Connection");
} // namespace httpoptions

class option
{
public:
	// 定义option_item类型.
	typedef std::pair<std::string, std::string> option_item;
	// 定义option_item_list类型.
	typedef std::vector<option_item> option_item_list;

public:
	option() {}
	~option() {}

public:

	// 这样就允许这样的应用:
	// http_stream s;
	// s.request_options(request_opts()("cookie","XXXXXX"));
	option & operator()(const std::string &key, const std::string &val)
	{
		insert(key, val);
		return *this;
	}

	// 添加选项, 由key/value形式添加.
	void insert(const std::string &key, const std::string &val)
	{
		m_opts.push_back(option_item(key, val));
	}

	// 删除选项.
	void remove(const std::string &key)
	{
		for (option_item_list::iterator i = m_opts.begin(); i != m_opts.end(); i++)
		{
			if (i->first == key)
			{
				m_opts.erase(i);
				return;
			}
		}
	}

	// 查找指定key的value.
	bool find(const std::string &key, std::string &val) const
	{
		std::string s = key;
		boost::to_lower(s);
		for (option_item_list::const_iterator f = m_opts.begin(); f != m_opts.end(); f++)
		{
			std::string temp = f->first;
			boost::to_lower(temp);
			if (temp == s)
			{
				val = f->second;
				return true;
			}
		}
		return false;
	}

	// 查找指定的 key 的 value. 没找到返回 ""，　这是个偷懒的帮助.
	std::string find(const std::string & key) const
	{
		std::string v;
		find(key,v);
		return v;
	}

	// 得到Header字符串.
	std::string header_string() const
	{
		std::string str;
		for (option_item_list::const_iterator f = m_opts.begin(); f != m_opts.end(); f++)
		{
			if (f->first != httpoptions::status_code)
				str += (f->first + ": " + f->second + "\r\n");
		}
		return str;
	}

	// 清空.
	void clear()
	{
		m_opts.clear();
	}

	// 返回所有option.
	option_item_list& option_all()
	{
		return m_opts;
	}

protected:
	option_item_list m_opts;
};

// 请求时的http选项.
// 以下选项为必http选项:
// _request_method, 取值 "GET/POST/HEAD", 默认为"GET".
// _request_body, 请求中的body内容, 取值任意, 默认为空.
// Host, 取值为http服务器, 默认为http服务器.
// Accept, 取值任意, 默认为"*/*".
typedef option request_opts;

// http服务器返回的http选项.
// 一般会包括以下几个选项:
// _status_code, http返回状态.
// Server, 服务器名称.
// Content-Length, 数据内容长度.
// Connection, 连接状态标识.
typedef option response_opts;

}

#endif // __OPTIONS_HPP__
