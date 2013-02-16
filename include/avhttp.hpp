//
// avhttp.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __AVHTTP_H__
#define __AVHTTP_H__

#pragma once

#include "avhttp/detail/abi_prefix.hpp"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifdef AVHTTP_ENABLE_OPENSSL
#include <boost/asio/ssl.hpp>
#include <openssl/x509v3.h>
#endif

using boost::asio::ip::tcp;

#include "avhttp/detail/error_codec.hpp"
#include "avhttp/url.hpp"
#include "avhttp/http_stream.hpp"

#include "avhttp/detail/abi_suffix.hpp"


#endif // __AVHTTP_H__
