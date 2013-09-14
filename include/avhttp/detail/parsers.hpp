//
// parsers.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2009 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// * $Id: parsers.hpp 49 2011-07-15 03:00:34Z jack $
//

#ifndef AVHTTP_PARSERS_HPP
#define AVHTTP_PARSERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <map>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <ctime>

#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>

#include "avhttp/settings.hpp"
#include "avhttp/detail/escape_string.hpp"

namespace avhttp {
namespace detail {

#ifndef atoi64
# ifdef _MSC_VER
#  define atoi64 _atoi64
# else
#  define atoi64(x) strtoll(x, (char**)NULL, 10)
# endif
#endif // atoi64

inline bool headers_equal(const std::string& a, const std::string& b)
{
	if (a.length() != b.length())
		return false;
	return std::equal(a.begin(), a.end(), b.begin(), tolower_compare);
}

inline void check_header(const std::string& name, const std::string& value,
	std::string& content_type, boost::int64_t& content_length,
	std::string& location)
{
	if (headers_equal(name, "Content-Type"))
		content_type = value;
	else if (headers_equal(name, "Content-Length"))
		content_length = atoi64(value.c_str());
	else if (headers_equal(name, "Location"))
		location = value;
}

#ifdef atoi64
#undef atoi64
#endif

template <typename Iterator>
bool parse_http_status_line(Iterator begin, Iterator end,
	int& version_major, int& version_minor, int& status)
{
	enum
	{
		http_version_h,
		http_version_t_1,
		http_version_t_2,
		http_version_p,
		http_version_slash,
		http_version_major_start,
		http_version_major,
		http_version_minor_start,
		http_version_minor,
		status_code_start,
		status_code,
		reason_phrase,
		linefeed,
		fail
	} state = http_version_h;

	Iterator iter = begin;
	std::string reason;
	while (iter != end && state != fail)
	{
		char c = *iter++;
		switch (state)
		{
		case http_version_h:
			state = (c == 'H') ? http_version_t_1 : fail;
			break;
		case http_version_t_1:
			state = (c == 'T') ? http_version_t_2 : fail;
			break;
		case http_version_t_2:
			state = (c == 'T') ? http_version_p : fail;
			break;
		case http_version_p:
			state = (c == 'P') ? http_version_slash : fail;
			break;
		case http_version_slash:
			state = (c == '/') ? http_version_major_start : fail;
			break;
		case http_version_major_start:
			if (is_digit(c))
			{
				version_major = version_major * 10 + c - '0';
				state = http_version_major;
			}
			else
				state = fail;
			break;
		case http_version_major:
			if (c == '.')
				state = http_version_minor_start;
			else if (is_digit(c))
				version_major = version_major * 10 + c - '0';
			else
				state = fail;
			break;
		case http_version_minor_start:
			if (is_digit(c))
			{
				version_minor = version_minor * 10 + c - '0';
				state = http_version_minor;
			}
			else
				state = fail;
			break;
		case http_version_minor:
			if (c == ' ')
				state = status_code_start;
			else if (is_digit(c))
				version_minor = version_minor * 10 + c - '0';
			else
				state = fail;
			break;
		case status_code_start:
			if (is_digit(c))
			{
				status = status * 10 + c - '0';
				state = status_code;
			}
			else
				state = fail;
			break;
		case status_code:
			if (c == ' ')
				state = reason_phrase;
			else if (is_digit(c))
				status = status * 10 + c - '0';
			else
				state = fail;
			break;
		case reason_phrase:
			if (c == '\r')
				state = linefeed;
			else if (is_ctl(c))
				state = fail;
			else
				reason.push_back(c);
			break;
		case linefeed:
			return (c == '\n');
		default:
			return false;
		}
	}
	return false;
}

template <typename Iterator>
bool parse_http_headers(Iterator begin, Iterator end,
	std::string& content_type, boost::int64_t& content_length,
	std::string& location)
{
	enum
	{
		first_header_line_start,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		linefeed,
		final_linefeed,
		fail
	} state = first_header_line_start;

	Iterator iter = begin;
	std::string reason;
	std::string name;
	std::string value;
	while (iter != end && state != fail)
	{
		char c = *iter++;
		switch (state)
		{
		case first_header_line_start:
			if (c == '\r')
				state = final_linefeed;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_line_start:
			if (c == '\r')
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				name.clear();
				value.clear();
				state = final_linefeed;
			}
			else if (c == ' ' || c == '\t')
				state = header_lws;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				name.clear();
				value.clear();
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_lws:
			if (c == '\r')
				state = linefeed;
			else if (c == ' ' || c == '\t')
				; // Discard character.
			else if (is_ctl(c))
				state = fail;
			else
			{
				state = header_value;
				value.push_back(c);
			}
			break;
		case header_name:
			if (c == ':')
				state = space_before_header_value;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
				name.push_back(c);
			break;
		case space_before_header_value:
			if (c == ' ')
				state = header_value;
			else if (is_ctl(c))
				state = fail;
			else
			{
				value.push_back(c);
				state = header_value;
			}
			break;
		case header_value:
			if (c == '\r')
				state = linefeed;
			else if (is_ctl(c))
				state = fail;
			else
				value.push_back(c);
			break;
		case linefeed:
			state = (c == '\n') ? header_line_start : fail;
			break;
		case final_linefeed:
			return (c == '\n');
		default:
			return false;
		}
	}
	return false;
}

typedef avhttp::option::option_item_list http_headers;

template <typename Iterator>
bool parse_http_headers(Iterator begin, Iterator end,
	std::string& content_type, boost::int64_t& content_length,
	std::string& location, http_headers& headers)
{
	enum
	{
		first_header_line_start,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		linefeed,
		final_linefeed,
		fail
	} state = first_header_line_start;

	Iterator iter = begin;
	std::string reason;
	std::string name;
	std::string value;
	while (iter != end && state != fail)
	{
		char c = *iter++;
		switch (state)
		{
		case first_header_line_start:
			if (c == '\r')
				state = final_linefeed;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_line_start:
			if (c == '\r')
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				headers.push_back(std::make_pair(name, value));
				name.clear();
				value.clear();
				state = final_linefeed;
			}
			else if (c == ' ' || c == '\t')
				state = header_lws;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				headers.push_back(std::make_pair(name, value));
				name.clear();
				value.clear();
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_lws:
			if (c == '\r')
				state = linefeed;
			else if (c == ' ' || c == '\t')
				; // Discard character.
			else if (is_ctl(c))
				state = fail;
			else
			{
				state = header_value;
				value.push_back(c);
			}
			break;
		case header_name:
			if (c == ':')
				state = space_before_header_value;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
				name.push_back(c);
			break;
		case space_before_header_value:
			if (c == ' ')
				state = header_value;
			if (c == '\r')	// 当value没有值的时候, 直接进入读取value完成逻辑, 避免失败.
				state = linefeed;
			else if (is_ctl(c))
				state = fail;
			else
			{
				value.push_back(c);
				state = header_value;
			}
			break;
		case header_value:
			if (c == '\r')
				state = linefeed;
			else if (is_ctl(c))
				state = fail;
			else
				value.push_back(c);
			break;
		case linefeed:
			state = (c == '\n') ? header_line_start : fail;
			break;
		case final_linefeed:
			return (c == '\n');
		default:
			return false;
		}
	}
	return false;
}

// 从Content-Disposition解析filename字段, 示例:
// attachment=other; filename="file.zip"; 这样的字符串中匹配到
// filename项, 将它的value保存到filename变量.
template <typename Iterator>
bool content_disposition_filename(Iterator begin, Iterator end, std::string& filename)
{
	enum
	{
		parse_key_start,
		parse_key,
		parse_value_start,
		parse_value,
		parse_fail
	} state = parse_key_start;

	Iterator iter = begin;
	std::string name;
	std::string value;
	char c;

	while (iter != end && state != parse_fail)
	{
		c = *iter++;
		switch (state)
		{
		case parse_key_start:
			if (c == ' ')
				continue;
			if (is_char(c))
			{
				name.push_back(c);
				state = parse_key;
			}
			else
				state = parse_fail;
			break;
		case parse_key:
			if (c == ';')
			{
				name = "";
				state = parse_key_start;
			}
			else if (c == '=')
			{
				value = "";
				state = parse_value_start;
			}
			else if (is_tspecial(c) || c == ':')
			{
				name = "";
				state = parse_key_start;
			}
			else if (is_char(c) || c == '_')
				name.push_back(c);
			break;
		case parse_value_start:
			if (c == ';' || c == '\"' || c == '\'')
				continue;
			if (is_char(c))
			{
				value.push_back(c);
				state = parse_value;
			}
			else
				state = parse_fail;
			break;
		case parse_value:
			if (c == ';' || c == '\"' || c == '\'')
			{
				if (name == "filename")
					filename = value;
				state = parse_key_start;
			}
			else if (is_char(c))
				value.push_back(c);
			else
				state = parse_fail;
			break;
		case parse_fail:
			break;
		}
	}
	if (name == "filename" && !value.empty())
		filename = value;
	if (filename.empty())
		return false;
	return true;
}

// 转换ptime到time_t.
inline time_t ptime_to_time_t(const boost::posix_time::ptime& pt)
{
	struct tm tm = boost::posix_time::to_tm(pt);
	return std::mktime(&tm);
}

namespace {

class gmt_time_input_face : public boost::posix_time::time_input_facet
{
	template <typename charT>
	inline static std::vector<std::basic_string<charT> >
	gather_month_strings(const std::locale& locale, bool short_strings=true)
	{
		typedef std::basic_string<charT> string_type;
		typedef std::vector<string_type> collection_type;
		typedef std::basic_ostringstream<charT> ostream_type;
		typedef std::ostreambuf_iterator<charT> ostream_iter_type;
		typedef std::basic_ostringstream<charT> stringstream_type;
		typedef std::time_put<charT>           time_put_facet_type;
		charT short_fmt[3] = { '%', 'b' };
		charT long_fmt[3]  = { '%', 'B' };
		collection_type months;
		string_type outfmt(short_fmt);
		if (!short_strings) {
			outfmt = long_fmt;
		}
		{
			//grab the needed strings by using the locale to
			//output each month
			const charT* p_outfmt = outfmt.c_str(), *p_outfmt_end = p_outfmt + outfmt.size();
			tm tm_value = { 0 };
			for (int m = 0; m < 12; m++) {
				tm_value.tm_mon = m;
				stringstream_type ss;
				ss.imbue(locale);
				ostream_iter_type oitr(ss);
				std::use_facet<time_put_facet_type>(locale).put(oitr, ss, ss.fill(),
					&tm_value,
					p_outfmt,
					p_outfmt_end);
				months.push_back(ss.str());
			}
		}
		return months;
	}

	template <typename charT>
	inline static std::vector<std::basic_string<charT> >
	gather_weekday_strings(const std::locale& locale, bool short_strings=true)
	{
		typedef std::basic_string<charT> string_type;
		typedef std::vector<string_type> collection_type;
		typedef std::basic_ostringstream<charT> ostream_type;
		typedef std::ostreambuf_iterator<charT> ostream_iter_type;
		typedef std::basic_ostringstream<charT> stringstream_type;
		typedef std::time_put<charT>           time_put_facet_type;
		charT short_fmt[3] = { '%', 'a' };
		charT long_fmt[3]  = { '%', 'A' };
		collection_type weekdays;
		string_type outfmt(short_fmt);
		if (!short_strings) {
			outfmt = long_fmt;
		}
		{
			//grab the needed strings by using the locale to
			//output each month / weekday
			const charT* p_outfmt = outfmt.c_str(), *p_outfmt_end = p_outfmt + outfmt.size();
			tm tm_value = { 0 };
			for (int i = 0; i < 7; i++) {
				tm_value.tm_wday = i;
				stringstream_type ss;
				ss.imbue(locale);
				ostream_iter_type oitr(ss);
				std::use_facet<time_put_facet_type>(locale).put(oitr, ss, ss.fill(),
					&tm_value,
					p_outfmt,
					p_outfmt_end);
				weekdays.push_back(ss.str());
			}
		}
		return weekdays;
	}

	class gmt_format_date_parser : public format_date_parser_type
	{
	public:
		gmt_format_date_parser(std::string fmt)
			: format_date_parser_type(fmt, std::locale::classic())
		{
			short_month_names(gmt_time_input_face::gather_month_strings<char>(std::locale::classic()));
			long_month_names(gmt_time_input_face::gather_month_strings<char>(std::locale::classic(), false));
			short_weekday_names(gmt_time_input_face::gather_weekday_strings<char>(std::locale::classic()));
			long_weekday_names(gmt_time_input_face::gather_weekday_strings<char>(std::locale::classic(), false));
		}
	};

public:
	gmt_time_input_face(std::string fmt)
		: boost::posix_time::time_input_facet(fmt)
	{
		m_parser = gmt_format_date_parser(fmt);
	}
};

} // namespace

// 解析http-date字符串.
// 注: 根据RFC2616规定HTTP-date 格式是 rfc1123-date | rfc850-date | asctime-date之一.
// 参数s 指定了需要解析的字符串.
// 参数t 返回解析出的time.
// 返回true表示解析成功, 返回false表示解析失败.
inline bool parse_http_date(const std::string& s, boost::posix_time::ptime& t)
{
	std::stringstream ss(s);
	gmt_time_input_face* rfc1123_date = new gmt_time_input_face("%a, %d %b %Y %H:%M:%S GMT");
	ss.imbue(std::locale(ss.getloc(), rfc1123_date));
	ss >> t;
	if (t != boost::posix_time::not_a_date_time)
	{
		return true;
	}
	ss.clear();
	ss.str(s);
	gmt_time_input_face* rfc850_date =
		new gmt_time_input_face("%A, %d-%b-%y %H:%M:%S GMT");
	ss.imbue(std::locale(ss.getloc(), rfc850_date));
	ss >> t;
	if (t != boost::posix_time::not_a_date_time)
	{
		return true;
	}
	ss.clear();
	ss.str(s);
	gmt_time_input_face* rfc850_date_workarround1 =
		new gmt_time_input_face("%A, %d-%b-%Y %H:%M:%S GMT");
	ss.imbue(std::locale(ss.getloc(), rfc850_date_workarround1));
	ss >> t;
	if (t != boost::posix_time::not_a_date_time)
	{
		return true;
	}
	ss.clear();
	ss.str(s);
	gmt_time_input_face* rfc850_date_workarround2 =
		new gmt_time_input_face("%a, %d-%b-%y %H:%M:%S GMT");
	ss.imbue(std::locale(ss.getloc(), rfc850_date_workarround2));
	ss >> t;
	if (t != boost::posix_time::not_a_date_time)
	{
		return true;
	}
	ss.clear();
	ss.str(s);
	gmt_time_input_face* asctime_date =
		new gmt_time_input_face("%a %b %d %H:%M:%S %Y");
	ss.imbue(std::locale(ss.getloc(), asctime_date));
	ss >> t;
	if (t != boost::posix_time::not_a_date_time)
	{
		return true;
	}

	return false;
}

// 解析http-date字符串.
// 注: 根据RFC2616规定HTTP-date 格式是 rfc1123-date | rfc850-date | asctime-date之一.
// 参数s 指定了需要解析的字符串.
// 参数t 返回解析出的UTC time.
// 返回true表示解析成功, 返回false表示解析失败.
inline bool parse_http_date(const std::string& s, time_t& t)
{
	boost::posix_time::ptime pt;
	if (!parse_http_date(s, pt))
	{
		return false;
	}
	t = ptime_to_time_t(pt);
	return true;
}

} // namespace detail
} // namespace avhttp

#endif // AVHTTP_PARSERS_HPP
