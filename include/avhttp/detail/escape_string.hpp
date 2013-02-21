#ifndef __ESCAPE_STRING_HPP__
#define __ESCAPE_STRING_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace avhttp {
namespace detail {

static const char hex_chars[] = "0123456789abcdef";

inline bool is_char(int c)
{
	return c >= 0 && c <= 127;
}

inline bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

inline bool is_ctl(int c)
{
	return (c >= 0 && c <= 31) || c == 127;
}

inline bool is_tspecial(int c)
{
	switch (c)
	{
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '/': case '[': case ']': case '?': case '=':
	case '{': case '}': case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

inline std::string to_hex(std::string const& s)
{
	std::string ret;
	for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		ret += hex_chars[((unsigned char)*i) >> 4];
		ret += hex_chars[((unsigned char)*i) & 0xf];
	}
	return ret;
}

inline void to_hex(char const *in, int len, char* out)
{
	for (char const* end = in + len; in < end; ++in)
	{
		*out++ = hex_chars[((unsigned char)*in) >> 4];
		*out++ = hex_chars[((unsigned char)*in) & 0xf];
	}
	*out = '\0';
}

inline bool is_print(char c)
{
	return c >= 32 && c < 127;
}

inline bool tolower_compare(char a, char b)
{
	return std::tolower(a) == std::tolower(b);
}

}
}

#endif // __ESCAPE_STRING_HPP__
