#ifndef __AVHTTP_H__
#define __AVHTTP_H__

#pragma once

// Disable some pesky MSVC warnings.
#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4127)
# pragma warning (disable:4251)
# pragma warning (disable:4355)
# pragma warning (disable:4512)
# pragma warning (disable:4996)
# pragma warning (disable:4267)
# pragma warning (disable:4819)
# pragma warning (disable:4244)
#endif // defined(_MSC_VER)

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#ifdef AVHTTP_ENABLE_OPENSSL
#include <boost/asio/ssl.hpp>
#include <openssl/x509v3.h>
#endif

using boost::asio::ip::tcp;


#include "avhttp/detail/abi_prefix.hpp"

#include "avhttp/detail/bind_protect.hpp"
#include "avhttp/detail/error_codec.hpp"
#include "avhttp/url.hpp"
#include "avhttp/http_stream.hpp"

#include "avhttp/detail/abi_suffix.hpp"

#if defined(_MSC_VER)
# pragma warning (pop)
#endif // defined(_MSC_VER)

#endif // __AVHTTP_H__
