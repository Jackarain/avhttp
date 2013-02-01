//
// socket_type.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
// Copyright (c) 2003, Arvid Norberg
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <string>
#include <algorithm>

namespace endian {

	template <class T> struct type {};

	// reads an integer from a byte stream in big endian byte order and converts
	// it to native endianess
	template <class T, class InIt>
	inline T read_impl(InIt& start, type<T>)
	{
		T ret = 0;
		for (int i = 0; i < (int)sizeof(T); ++i)
		{
			ret <<= 8;
			ret |= static_cast<uint8_t>(*start);
			++start;
		}
		return ret;
	}

	template <class InIt>
	uint8_t read_impl(InIt& start, type<uint8_t>)
	{
		return static_cast<uint8_t>(*start++);
	}

	template <class InIt>
	int8_t read_impl(InIt& start, type<int8_t>)
	{
		return static_cast<int8_t>(*start++);
	}

	template <class T, class OutIt>
	inline void write_impl(T val, OutIt& start)
	{
		for (int i = (int)sizeof(T)-1; i >= 0; --i)
		{
			*start = static_cast<unsigned char>((val >> (i * 8)) & 0xff);
			++start;
		}
	}

	// -- adaptors

	template <class InIt>
	int64_t read_int64(InIt& start)
	{ return read_impl(start, type<int64_t>()); }

	template <class InIt>
	uint64_t read_uint64(InIt& start)
	{ return read_impl(start, type<uint64_t>()); }

	template <class InIt>
	uint32_t read_uint32(InIt& start)
	{ return read_impl(start, type<uint32_t>()); }

	template <class InIt>
	int32_t read_int32(InIt& start)
	{ return read_impl(start, type<int32_t>()); }

	template <class InIt>
	int16_t read_int16(InIt& start)
	{ return read_impl(start, type<int16_t>()); }

	template <class InIt>
	uint16_t read_uint16(InIt& start)
	{ return read_impl(start, type<uint16_t>()); }

	template <class InIt>
	int8_t read_int8(InIt& start)
	{ return read_impl(start, type<int8_t>()); }

	template <class InIt>
	uint8_t read_uint8(InIt& start)
	{ return read_impl(start, type<uint8_t>()); }


	template <class OutIt>
	void write_uint64(uint64_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_int64(int64_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_uint32(uint32_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_int32(int32_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_uint16(uint16_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_int16(int16_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_uint8(uint8_t val, OutIt& start)
	{ write_impl(val, start); }

	template <class OutIt>
	void write_int8(int8_t val, OutIt& start)
	{ write_impl(val, start); }

	inline void write_string(std::string const& str, char*& start)
	{
		std::memcpy((void*)start, str.c_str(), str.size());
		start += str.size();
	}

	template <class OutIt>
	void write_string(std::string const& str, OutIt& start)
	{
		std::copy(str.begin(), str.end(), start);
	}
}
