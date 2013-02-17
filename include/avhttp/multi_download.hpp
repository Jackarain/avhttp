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
#include <boost/filesystem.hpp>

#include "http_stream.hpp"

/************************************************************************

boost::asio::io_service io;

multi_download mget(io);

settings s;

mget.open(url, s); // async_open(url, s);
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

namespace fs = boost::filesystem;

// 下载设置.
enum downlad_mode
{
	compact_mode,		// 紧凑模式下载.
	dispersion_mode		// 松散模式下载.
};

struct settings
{
	int m_download_rate_limit;		// 下载速率限制, -1为无限制.
	int m_connections_limit;		// 连接数限制, -1为无限制.
	int m_piece_size;				// 分块大小, 默认根据文件大小自动计算.
	downlad_mode m_downlad_mode;	// 下载模式, 默认为dispersion_mode.
	fs::path m_meta_file;			// meta_file路径, 默认为当前路径下同文件名的.meta文件.
};

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
