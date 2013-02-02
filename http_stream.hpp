//
// http_stream.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Jack (jack.wgm@gmail.com)
//

#ifndef __HTTP_STREAM_HPP__
#define __HTTP_STREAM_HPP__

#pragma once

#include "detail/abi_prefix.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include "url.hpp"
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

	///打开一个指定的url.
	// 失败将抛出一个boost::system::system_error异常.
	// @param u 将要打开的URL.
	// @begin example
	//   avhttp::http_stream h_stream(io_service);
	//   try
	//   {
	//     h_stream.open("http://www.boost.org");
	//   }
	//   catch (boost::system::error_code& e)
	//   {
	//     std::cerr << e.waht() << std::endl;
	//   }
	// @end example
	void open(const url &u)
	{

	}

	///打开一个指定的url.
	// @param u 将要打开的URL.
	// 通过ec引用获得执行状态.
	// @begin example
	//   avhttp::http_stream h_stream(io_service);
	//   boost::system::error_code ec;
	//   h_stream.open("http://www.boost.org", ec);
	//   if (ec)
	//   {
	//     std::cerr << e.waht() << std::endl;
	//   }
	// @end example
	void open(const url &u, boost::system::error_code &ec)
	{

	}

	///异步打开一个指定的URL.
	// @param u 将要打开的URL.
	// @param handler 将被调用在打开完成时. 它必须满足以下条件:
	// @begin code
	//  void handler(
	//    const boost::system::error_code& ec // 用于返回操作状态.
	//  );
	// @end code
	// @begin example
	//  void open_handler(const boost::system::error_code& ec)
	//  {
	//    if (!ec)
	//    {
	//      // 打开成功!
	//    }
	//  }
	//  ...
	//  avhttp::http_stream h_stream(io_service);
	//  h_stream.async_open("http://www.boost.org", open_handler);
	// @end example
	// @备注: handler也可以使用boost.bind来绑定一个符合规定的函数作
	// 为async_open的参数handler.
	template <typename Handler>
	void async_open(const url &u, Handler handler)
	{

	}

	// request
	// open
	// async_open
	// read_some
	// async_read_some


protected:
	boost::asio::io_service &m_io_service;
	request_opts m_req_opts;						// 向http服务器请求的头信息.
	response_opts m_resp_opts;						// http服务器返回的http头信息.
};

}

#include "detail/abi_suffix.hpp"

#endif // __HTTP_STREAM_HPP__
