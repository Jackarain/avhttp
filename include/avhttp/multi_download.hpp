//
// multi_download.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MULTI_DOWNLOAD_HPP__
#define MULTI_DOWNLOAD_HPP__

#pragma once

#include <boost/noncopyable.hpp>

#include "http_stream.hpp"

/************************************************************************

boost::asio::io_service io;

multi_download mget(io);
mget.open(url); // async_open(url);
mget.close();

struct handle_reader
{
	boost::uint64_t offset;
	std::size_t size;
	std::vector<char> data;
};

boost::system::error_code ec;
handle_reader hr;
mget.read_some(hr, ec);

mget.async_read_some(hr, handler);

void handler(boost::system::error_code ec)
{
}

************************************************************************/

namespace avhttp
{

class multi_download : public boost::noncopyable
{
public:
	multi_download() {}
	~multi_download() {}

public:

private:
};

} // avhttp

#endif // MULTI_DOWNLOAD_HPP__
