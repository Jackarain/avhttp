//
// avhttp.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __AVHTTP_HPP__
#define __AVHTTP_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "avhttp/detail/abi_prefix.hpp"

#include <boost/version.hpp>
#include <boost/static_assert.hpp>

namespace
{
	// 由于avhttp使用了boost.locale, 必须使用boost版本1.48或以上版本!!!
	// 原因是boost.locale是在boost-1.48加入boost的.
	BOOST_STATIC_ASSERT_MSG(BOOST_VERSION >= 104800, "You must use boost-1.48 or later!!!");
}

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

// 仅限于header only, 如果avhttp支持单独编译时或编译成动态库, AVHTTP_DECL应该
// 相应支持其它定义.
#ifndef AVHTTP_DECL
# define AVHTTP_DECL inline
#endif

#include "avhttp/logging.hpp"
#include "avhttp/detail/error_codec.hpp"
#include "avhttp/entry.hpp"
#include "avhttp/bencode.hpp"
#include "avhttp/url.hpp"
#include "avhttp/http_stream.hpp"
#include "avhttp/rangefield.hpp"
#include "avhttp/bitfield.hpp"
#include "avhttp/multi_download.hpp"

#include "avhttp/detail/abi_suffix.hpp"


#endif // __AVHTTP_HPP__
