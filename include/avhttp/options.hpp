//
// options.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#pragma once

#include <map>
#include <string>

namespace avhttp {

// 如果没有定义最大重定向次数, 则默认为5次最大重定向.
#ifndef AVHTTP_MAX_REDIRECTS
#define AVHTTP_MAX_REDIRECTS 5
#endif


// 选项表.
typedef std::map<std::string, std::string> option_item;

namespace httpoptions{
	// 定义一些常用的　http 选项为 const string , 这样就不用记忆那些单词了，呵呵.
	static const std::string request_method("_request_method");
	static const std::string request_body("_request_body");
	static const std::string cookie("cookie");
	static const std::string referer("referer");
	static const std::string content_type("content-type");
}

class option
{
public:
	option() {}
	~option() {}

// public:

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
		m_opts[key] = val;
	}

	// 删除选项.
	void remove(const std::string &key)
	{
		option_item::iterator f = m_opts.find(key);
		if (f != m_opts.end())
			m_opts.erase(f);
	}

	// 查找指定key的value.
	bool find(const std::string &key, std::string &val)
	{
		option_item::iterator f = m_opts.find(key);
		if (f == m_opts.end())
			return false;
		val = f->second;
		return true;
	}

	// 清空.
	void clear()
	{
		m_opts.clear();
	}

	// 返回所有option.
	option_item& option_all()
	{
		return m_opts;
	}

protected:
	option_item m_opts;
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

#endif // __OPTIONS_H__
