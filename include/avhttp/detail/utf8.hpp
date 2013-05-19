//
// utf8.hpp
// ~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __UTF8_HPP__
#define __UTF8_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>

#include <boost/locale.hpp>
#include <boost/locale/utf.hpp>

namespace avhttp {
namespace detail {

// 默认字符集编码.
const static std::string default_characters = "GBK";

// 字符集编码转换接口声明.
inline std::string wide_utf8(const std::wstring &source);

inline std::wstring utf8_wide(std::string const &source);

inline std::string ansi_utf8(
	std::string const &source, const std::string &characters = default_characters);

inline std::string utf8_ansi(
	std::string const &source, const std::string &characters = default_characters);

inline std::string wide_ansi(
	std::wstring const &source, const std::string &characters = default_characters);

inline std::wstring ansi_wide(
	const std::string &source, const std::string &characters = default_characters);

// 字符集编码转换接口实现, 使用boost实现.
inline std::wstring ansi_wide(
	const std::string &source, const std::string &characters/* = default_characters*/)
{
	std::wstring destination;

	std::string s = ansi_utf8(source, characters);
	destination = boost::locale::conv::utf_to_utf<wchar_t>(s);

	return destination;
}

inline std::string ansi_utf8(
	std::string const &source, const std::string &characters/* = default_characters*/)
{
	std::string destination;

	destination = boost::locale::conv::between(source, "UTF-8", characters);

	return destination;
}

inline std::string wide_utf8(const std::wstring &source)
{
	std::string destination;

	destination = boost::locale::conv::utf_to_utf<char>(source);

	return destination;
}

inline std::wstring utf8_wide(std::string const &source)
{
	std::wstring destination;

	destination = boost::locale::conv::utf_to_utf<wchar_t>(source);

	return destination;
}

inline std::string utf8_ansi(
	std::string const &source, const std::string &characters/* = default_characters*/)
{
	std::string destination;

	destination = boost::locale::conv::between(source, characters, "UTF-8");

	return destination;
}

inline std::string wide_ansi(
	std::wstring const &source, const std::string &characters/* = default_characters*/)
{
	std::string destination;

	destination = wide_utf8(source);
	destination = utf8_ansi(destination, characters);

	return destination;
}

} // namespace detail
} // namespace avhttp

#endif // __UTF8_HPP__
