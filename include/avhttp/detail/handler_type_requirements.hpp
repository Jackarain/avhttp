//
// detail/handler_type_requirements.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
// Copyright (c) 2003, Arvid Norberg
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __HANDLER_TYPE_REQUIREMENTS_HPP__
#define __HANDLER_TYPE_REQUIREMENTS_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)

#define BOOST_ASIO_OPEN_HANDLER_CHECK( \
    handler_type, handler) \
  \
  BOOST_ASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(boost::asio::detail::one_arg_handler_test( \
          handler, \
          static_cast<const boost::system::error_code*>(0))) == 1, \
      "OpenHandler type requirements not met") \
  \
  typedef boost::asio::detail::handler_type_requirements< \
      sizeof( \
        boost::asio::detail::argbyv( \
          boost::asio::detail::clvref(handler))) + \
      sizeof( \
        boost::asio::detail::lvref(handler)( \
          boost::asio::detail::lvref<const boost::system::error_code>()), \
        char(0))>

#define BOOST_ASIO_REQUEST_HANDLER_CHECK( \
    handler_type, handler) \
  \
  BOOST_ASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(boost::asio::detail::one_arg_handler_test( \
          handler, \
          static_cast<const boost::system::error_code*>(0))) == 1, \
      "OpenHandler type requirements not met") \
  \
  typedef boost::asio::detail::handler_type_requirements< \
      sizeof( \
        boost::asio::detail::argbyv( \
          boost::asio::detail::clvref(handler))) + \
      sizeof( \
        boost::asio::detail::lvref(handler)( \
          boost::asio::detail::lvref<const boost::system::error_code>()), \
        char(0))>

#else

#define BOOST_ASIO_OPEN_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int

#endif

} // namespace detail
} // namespace asio
} // namespace boost

#endif // __HANDLER_TYPE_REQUIREMENTS_HPP__
