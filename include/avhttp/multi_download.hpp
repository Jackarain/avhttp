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

#include <vector>
#include <list>

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

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

// 下载模式.
enum downlad_mode
{
	// 紧凑模式下载, 紧凑模式是指, 将文件分片后, 从文件头开始, 一片紧接着一片,
	// 连续不断的下载.
	compact_mode,

	// 松散模式下载, 是指将文件分片后, 按连接数平分为N大块进行下载.
	dispersion_mode
};

// 下载设置.
struct settings
{
	int m_download_rate_limit;		// 下载速率限制, -1为无限制.
	int m_connections_limit;		// 连接数限制, -1为无限制.
	int m_piece_size;				// 分块大小, 默认根据文件大小自动计算.
	downlad_mode m_downlad_mode;	// 下载模式, 默认为dispersion_mode.
	fs::path m_meta_file;			// meta_file路径, 默认为当前路径下同文件名的.meta文件.
};

// 数据读取handler定义.
// 用于multi_download读取数据, 其中包含了数据的偏移, 以及数据大小, 和数据缓冲.
struct handle_reader
{
	boost::uint64_t m_offset;	// 数据偏移.
	std::size_t m_size;			// 数据大小.
	std::vector<char> m_data;	// 数据缓冲.
};


class multi_download : public boost::noncopyable
{
	typedef boost::shared_ptr<http_stream> http_stream_ptr;
public:
	multi_download(boost::asio::io_service &io)
		: m_io_service(io)
	{}
	~multi_download()
	{}

public:

private:
	boost::asio::io_service &m_io_service;	// io_service引用.
	std::list<http_stream_ptr> m_streams;	// http_stream容器, 每一个http_stream是一个http连接.
};

} // avhttp

#endif // MULTI_DOWNLOAD_HPP__
