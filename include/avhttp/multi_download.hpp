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

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/lexical_cast.hpp>

#include "storage_interface.hpp"
#include "file.hpp"
#include "http_stream.hpp"

namespace avhttp
{

// 下载模式.
enum downlad_mode
{
	// 紧凑模式下载, 紧凑模式是指, 将文件分片后, 从文件头开始, 一片紧接着一片,
	// 连续不断的下载.
	compact_mode,

	// 松散模式下载, 是指将文件分片后, 按连接数平分为N大块进行下载.
	dispersion_mode,

	// 快速读取模式下载, 这个模式是根据用户读取数据位置开始下载数据, 是尽快响应
	// 下载用户需要的数据.
	quick_read_mode
};

// 下载设置.
struct settings
{
	settings ()
		: m_download_rate_limit(-1)
		, m_connections_limit(-1)
		, m_piece_size(-1)
		, m_downlad_mode(dispersion_mode)
	{}

	int m_download_rate_limit;			// 下载速率限制, -1为无限制.
	int m_connections_limit;			// 连接数限制, -1为无限制.
	int m_piece_size;					// 分块大小, 默认根据文件大小自动计算.
	downlad_mode m_downlad_mode;		// 下载模式, 默认为dispersion_mode.
	fs::path m_meta_file;				// meta_file路径, 默认为当前路径下同文件名的.meta文件.
};

// 重定义http_stream_ptr指针.
typedef boost::shared_ptr<http_stream> http_stream_ptr;

// 定义http_stream_obj.
struct http_stream_obj
{
	http_stream_ptr m_stream;				// http_stream对象.
	boost::array<char, 2048> m_buffer;		// 数据缓冲.
};


class multi_download : public boost::noncopyable
{
public:
	multi_download(boost::asio::io_service &io)
		: m_io_service(io)
	{}
	~multi_download()
	{}

public:
	void open(const url &u, boost::system::error_code &ec)
	{
		settings s;
		ec = open(u, s);
	}

	boost::system::error_code open(const url &u, const settings &s, storage_constructor_type p = NULL)
	{
		boost::system::error_code ec;

		// 清空所有连接.
		m_streams.clear();
		m_file_size = -1;

		// 创建一个http_stream对象.
		http_stream_obj obj;

		request_opts req_opt;
		req_opt.insert("Range", "bytes=0-");

		// 创建http_stream并同步打开, 检查返回状态码是否为206, 如果非206则表示该http服务器不支持多点下载.
		obj.m_stream.reset(new http_stream(m_io_service));
		http_stream &h = *obj.m_stream;
		h.request_options(req_opt);
		h.open(u, ec);
		// 打开失败则退出.
		if (ec)
		{
			return ec;
		}

		// 保存最终url信息.
		std::string location = h.location();
		if (!location.empty())
			m_final_url = location;
		else
			m_final_url = u;

		// 判断是否支持多点下载.
		std::string status_code;
		h.response_options().find("_status_code", status_code);
		if (status_code != "206")
			m_accept_multi = false;
		else
			m_accept_multi = true;

		// 得到文件大小.
		if (m_accept_multi)
		{
			std::string length;
			h.response_options().find("Content-Length", length);
			if (length.empty())
			{
				h.response_options().find("Content-Range", length);
				std::string::size_type f = length.find('/');
				if (f++ != std::string::npos)
					length = length.substr(f);
				else
					length = "";
				if (length.empty())
				{
					// 得到不文件长度, 设置为不支持多下载模式.
					m_accept_multi = false;
				}
			}

			if (!length.empty())
			{
				try
				{
					m_file_size = boost::lexical_cast<boost::int64_t>(length);
				}
				catch (boost::bad_lexical_cast &)
				{
					// 得不到正确的文件长度, 设置为不支持多下载模式.
					m_accept_multi = false;
				}
			}
		}

		// 创建存储对象.
		if (!p)
			m_storage.reset(default_storage_constructor());
		else
			m_storage.reset(p());
		BOOST_ASSERT(m_storage);

		// 保存这个http_stream连接.
		m_streams.push_back(obj);

		// 如果支持多点下载, 按设置创建其它http_stream.

		// 开始发起异步数据请求.
	}

protected:


private:
	boost::asio::io_service &m_io_service;	// io_service引用.
	std::list<http_stream_obj> m_streams;	// 每一个http_stream_obj是一个http连接.
	url m_final_url;						// 最终的url, 如果有跳转的话, 是跳转最后的那个url.
	bool m_accept_multi;					// 是否支持多点下载.
	boost::int64_t m_file_size;				// 文件大小, 如果没有文件大小值为-1.
	settings m_settings;					// 当前用户设置.
	boost::scoped_ptr<storage_interface> m_storage;
};

} // avhttp

#endif // MULTI_DOWNLOAD_HPP__
