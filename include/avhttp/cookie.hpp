//
// cookie.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __COOKIE_HPP__
#define __COOKIE_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <map>
#include <boost/date_time.hpp>
#include "avhttp/detail/escape_string.hpp"
#include "avhttp/detail/parsers.hpp"

namespace avhttp {

/*

boost::asio::io_service io;
http_stream h(io);

cookies c;
c("key=value;key2=value2");
c("key", "value");

h.set_cookies(c);

*/

class cookies
{
	typedef struct cookie_t
	{
		cookie_t()
			: httponly(false)
			, secure(false)
		{}

		// name和value, name必须不为空串.
		std::string name;
		std::string value;

		// 如果为空串, 即匹配任何域名.
		std::string domain;

		// 如果为空串, 即匹配任何路径.
		std::string path;

		// 如果为无效时间, 表示为不过期.
		boost::posix_time::ptime expires;

		// httponly在这里没有意义, secure只有为https时, 才带上此cookie.
		bool httponly;
		bool secure;
	} http_cookie;

public:

	explicit cookies()
	{}
	~cookies()
	{}

	struct const_iterator
	{
		friend class cookies;

		typedef http_cookie value_type;
		typedef ptrdiff_t difference_type;
		typedef http_cookie const* pointer;
		typedef http_cookie& reference;
		typedef std::forward_iterator_tag iterator_category;

		http_cookie& operator*() { return m_iter->second; }
		http_cookie* operator->() { return &(m_iter->second); }
		const_iterator& operator++() { m_iter++; return *this; }
		const_iterator operator++(int)
		{ const_iterator ret(*this); m_iter++; return ret; }
		const_iterator& operator--() { m_iter--; return *this; }
		const_iterator operator--(int)
		{ const_iterator ret(*this); m_iter--; return ret; }

		bool operator==(const_iterator const& rhs) const
		{ return m_iter == rhs.m_iter; }

		bool operator!=(const_iterator const& rhs) const
		{ return m_iter != rhs.m_iter; }

		const_iterator(std::map<std::string, http_cookie>::iterator& iter)
			: m_iter(iter)
		{}

	private:
		std::map<std::string, http_cookie>::iterator m_iter;
	};

	const_iterator begin() const { return const_iterator(m_cookies.begin()); }
	const_iterator end() const { return const_iterator(m_cookies.end()); }

	// 添加一个cookie.
	cookies& operator()(const std::string& key, const std::string& value)
	{
		// m_cookies.insert(std::make_pair(key, value));
		return *this;
	}

	// 添加或更新字符串中包含的cookie, 如果cookie过期将被删除.
	cookies& operator()(const std::string& str)
	{
		// m_cookies.insert(std::make_pair(key, value));
		std::vector<http_cookie> cookie;
		if (parse_cookie_string(str, cookie))
		{
			for (std::vector<http_cookie>::iterator i = cookie.begin();
				i != cookie.end(); i++)
			{
				// 过期的cookie, 删除之.
				if (i->expires < boost::posix_time::second_clock::local_time())
				{
					std::map<std::string, http_cookie>::iterator finder = m_cookies.find(i->name);
					if (finder != m_cookies.end())
					{
						m_cookies.erase(finder);
					}
				}
				else
				{
					m_cookies[i->name] = *i;	// 更新或插入cookie.
				}
			}
		}

		return *this;
	}

	// 返回cookie数.
	int size() const
	{
		return m_cookies.size();
	}

private:

	// 解析cookie字符.
	// 示例字符串:
	// gsid=none; expires=Sun, 22-Sep-2013 14:27:43 GMT; path=/; domain=.fidelity.cn; httponly
	// gsid=none; gsid2=none; expires=Sun, 22-Sep-2013 14:27:43 GMT; path=/; domain=.fidelity.cn
	bool parse_cookie_string(const std::string& str, std::vector<http_cookie>& cookie)
	{
		// 解析状态.
		enum
		{
			cookie_name_start,
			cookie_name,
			cookie_value_start,
			cookie_value,
			cookie_bad
		} state = cookie_name_start;

		std::string::const_iterator iter = str.begin();
		std::string name;
		std::string value;
		std::map<std::string, std::string> tmp;
		http_cookie cookie_tmp;

		// 开始解析http-cookie字符串.
		while (iter != str.end() && state != cookie_bad)
		{
			const char c = *iter++;
			switch (state)
			{
			case cookie_name_start:
				if (c == ' ')
					continue;
				if (detail::is_char(c))
				{
					name.push_back(c);
					state = cookie_name;
				}
				else
					state = cookie_bad;
				break;
			case cookie_name:
				if (c == ';')
				{
					state = cookie_name_start;
					if (name == "secure")
						cookie_tmp.secure = true;
					else if (name == "httponly")
						cookie_tmp.httponly = true;
					else
						state = cookie_bad;
					name = "";
				}
				else if (c == '=')
				{
					value = "";
					state = cookie_value_start;
				}
				else if (detail::is_tspecial(c) || c == ':')
				{
					name = "";
					state = cookie_name_start;
				}
				else if (detail::is_char(c) || c == '_')
					name.push_back(c);
				break;
			case cookie_value_start:
				if (c == ';' || c == '\"' || c == '\'')
					continue;
				if (detail::is_char(c))
				{
					value.push_back(c);
					state = cookie_value;
				}
				else
					state = cookie_bad;
				break;
			case cookie_value:
				if (c == ';' || c == '\"' || c == '\'')
				{
					tmp[name] = value;	// 添加或更新.
					name = "";
					value = "";
					state = cookie_name_start;
				}
				else if (detail::is_char(c))
					value.push_back(c);
				else
					state = cookie_bad;
				break;
			case cookie_bad:
				break;
			}
		}
		if (state == cookie_name && !name.empty())
		{
			if (name == "secure")
				cookie_tmp.secure = true;
			else if (name == "httponly")
				cookie_tmp.httponly = true;
		}
		else if (state == cookie_value && !value.empty())
		{
			tmp[name] = value;	// 添加或更新.
		}
		else if (state == cookie_bad)
		{
			return false;
		}

		for (std::map<std::string, std::string>::iterator i = tmp.begin();
			i != tmp.end(); )
		{
			if (i->first == "expires")
			{
				if (!detail::parse_http_date(i->second, cookie_tmp.expires))
					BOOST_ASSERT(0);	// for debug.
				tmp.erase(i++);
			}
			else if (i->first == "domain")
			{
				cookie_tmp.domain = i->second;
				tmp.erase(i++);
			}
			else if (i->first == "path")
			{
				cookie_tmp.path = i->second;
				tmp.erase(i++);
			}
			else
			{
				i++;
			}
		}

		// 添加到容器返回.
		for (std::map<std::string, std::string>::iterator i = tmp.begin();
			i != tmp.end(); i++)
		{
			cookie_tmp.name = i->first;
			cookie_tmp.value = i->second;
			cookie.push_back(cookie_tmp);
		}

		return true;
	}

private:
	// 保存所有cookie.
	mutable std::map<std::string, http_cookie> m_cookies;
};

} // namespace avhttp

#endif // __COOKIE_HPP__
