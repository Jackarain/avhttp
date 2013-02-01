#ifndef __HTTP_STREAM_HPP__
#define __HTTP_STREAM_HPP__

#pragma once

#include <boost/noncopyable.hpp>
#ifdef WIN32
#pragma warning(disable: 4267)	// 禁用VC警告.
#endif // WIN32
#include <boost/asio.hpp>

#include "options.hpp"

namespace avhttp {

// 一个http流类实现, 用于同步或异步访问http数据.
// 
class http_stream : public boost::noncopyable
{
public:
	http_stream(boost::asio::io_service &io)
		: m_io_service(io)
	{}

	virtual ~http_stream()
	{}

public:

	// request
	// open
	// read_some
	// async_read_some


protected:
	boost::asio::io_service &m_io_service;
	options m_opts;
};

}


#endif // __HTTP_STREAM_HPP__
