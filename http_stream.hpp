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

#include <vector>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#ifdef HTTP_ENABLE_OPENSSL
#include <boost/asio/ssl.hpp>
#include <openssl/x509v3.h>
#endif

using boost::asio::ip::tcp;

#include "url.hpp"
#include "options.hpp"
#include "detail/parsers.hpp"
#include "detail/error_codec.hpp"

namespace avhttp {

// 一个http流类实现, 用于同步或异步访问http数据.
//
class http_stream : public boost::noncopyable
{
public:
	http_stream(boost::asio::io_service &io)
		: m_io_service(io)
		, m_socket(io)
#ifdef HTTP_ENABLE_OPENSSL
		, m_ssl_context(m_io_service, boost::asio::ssl::context::sslv23)
		, m_ssl_socket(m_io_service, m_ssl_context)
#endif
		, m_keep_alive(false)
		, m_status_code(-1)
		, m_content_length(0)
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
		boost::system::error_code ec;
		open(u, ec);
		if (ec)
		{
			boost::throw_exception(boost::system::system_error(ec));
		}
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
		const std::string protocol = u.protocol();

		// 保存url.
		m_url = u;

		// 清空一些选项.
		m_response_opts.clear();
		m_request_opts.clear();
		m_content_type = "";
		m_status_code = 0;

		if (protocol == "http")
		{
			// 保存协议类型.
			m_protocol = "http";

			// 开始进行连接.
			if (!http_socket().is_open())
			{
				// 开始解析端口和主机名.
				tcp::resolver resolver(m_io_service);
				std::ostringstream port_string;
				port_string << m_url.port();
				tcp::resolver::query query(m_url.host(), port_string.str());
				tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
				tcp::resolver::iterator end;

				// 尝试连接解析出来的服务器地址.
				ec = boost::asio::error::host_not_found;
				while (ec && endpoint_iterator != end)
				{
					http_socket().close(ec);
					http_socket().connect(*endpoint_iterator++, ec);
				}
				if (ec)
				{
					return;
				}
				// 禁用Nagle在socket上.
				http_socket().set_option(tcp::no_delay(true), ec);
				if (ec)
				{
					return;
				}
			}
			else
			{
				// socket已经打开.
				ec = boost::asio::error::already_open;
				return;
			}

			// 发出请求.
			request(m_request_opts);

			// 循环读取.
			for (;;)
			{
				boost::asio::read_until(http_socket(), m_response, "\r\n", ec);
				if (ec)
				{
					return;
				}

				// 检查http状态码.
				int version_major = 0;
				int version_minor = 0;
				if (!detail::parse_http_status_line(
					std::istreambuf_iterator<char>(&m_response),
					std::istreambuf_iterator<char>(),
					version_major, version_minor, m_status_code))
				{
					ec = avhttp::errc::malformed_status_line;
					return;
				}

				// "continue"表示我们需要继续等待接收状态.
				if (m_status_code != avhttp::errc::continue_request)
					break;
			} // end for.

			// 添加状态码.
			m_response_opts.insert("status_code", boost::str(boost::format("%d") % m_status_code));

			// 接收掉所有Http Header.
			std::size_t bytes_transferred = boost::asio::read_until(http_socket(), m_response, "\r\n\r\n", ec);
			if (ec)
			{
				return;
			}

			std::string header_string;
			std::string header_line;
			std::istream response_stream(&m_response);

			// 解析到m_response_opts.
			while (std::getline(response_stream, header_line) && header_line != "\r")
			{
				header_string += header_line + "\n";
				std::vector<std::string> item;
				boost::split(item, header_line, boost::is_any_of(":"), boost::token_compress_on);
				if (item.size() == 2)
				{
					boost::trim(item[0]);
					boost::trim(item[1]);
					m_response_opts.insert(item[0], item[1]);
				}
			}
			header_string += "\r\n";

			// 解析Http Header.
			if (!detail::parse_http_headers(header_string.begin(), header_string.end(),
				m_content_type, m_content_length, m_location))
			{
				ec = avhttp::errc::malformed_response_headers;
				return;
			}

			// 判断是否需要跳转.
			if (ec == avhttp::errc::moved_permanently || ec == avhttp::errc::found)
			{
				http_socket().close(ec);
				open(u, ec);
			}


		}
#ifdef HTTP_ENABLE_OPENSSL
		else if (protocol == "https")
		{
			m_protocol = "https";
		}
#endif
		else
		{
			ec = boost::asio::error::operation_not_supported;
			return;
		}
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



	///判断是否打开.
	// @返回是否打开.
	bool is_open() const
	{
		return false;
	}

	///设置请求时的http选项.
	// @param options 为http的选项. 目前有以下几项特定选项:
	//  request_method, 取值 "GET/POST/HEAD", 默认为"GET".
	//  Host, 取值为http服务器, 默认为http服务器.
	//  Accept, 取值任意, 默认为"*/*".
	// @begin example
	//  avhttp::http_stream h_stream(io_service);
	//  request_opts options;
	//  options.insert("request_method", "POST"); // 默认为GET方式.
	//  h_stream.request_options(options);
	//  ...
	// @end example
	void request_options(const request_opts& options)
	{
		m_request_opts = options;
	}

	///返回请求时的http选项.
	// @begin example
	//  avhttp::http_stream h_stream(io_service);
	//  request_opts options;
	//  options = h_stream.request_options();
	//  ...
	// @end example
	request_opts request_options(void) const
	{
		return m_request_opts;
	}

	///得到底层引用.
	// @函数返回底层socket的引用, 失败将抛出一个boost::system::system_error异常.
	// @begin example
	//  avhttp::http_stream h_stream(io_service);
	//  tcp::socket &sock = h_stream.lowest_layer("http");
	//  ...
	// @end example
	tcp::socket& lowest_layer(const std::string &protocol)
	{
		if (protocol == "http")
		{
			return m_socket;
		}
#ifdef HTTP_ENABLE_OPENSSL
		if (protocol == "https")
		{
			return m_ssl_socket.lowest_layer();
		}
#endif
		// 未知的协议, 抛出operation_not_supported异常.
		boost::system::system_error ex(boost::asio::error::operation_not_supported);
		boost::throw_exception(ex);
	}

	///得到socket的引用.
	// @函数返回底层socket的引用, 失败将抛出一个boost::system::system_error异常.
	// @begin example
	//  avhttp::http_stream h_stream(io_service);
	//  tcp::socket &sock = h_stream.http_socket();
	//  ...
	// @end example
	inline tcp::socket& http_socket()
	{
		return lowest_layer(m_protocol);
	}

	///向http服务器发起一个请求.
	// @向http服务器发起一个请求, 如果失败抛出异常.
	// @param opt是向服务器发起请求的选项信息.
	// @begin example
	//  avhttp::http_stream h_stream(io_service);
	//  ...
	//  request_opts opt;
	//  opt.insert("cookie", "name=admin;passwd=#@aN@2*242;");
	//  ...
	//  h_stream.request(opt);
	// @end example
	void request(request_opts &opt)
	{
		boost::system::error_code ec;
		option_item opts = opt.option_all();

		// 判断socket是否打开.
		if (!http_socket().is_open())
		{
			boost::system::system_error ex(boost::asio::error::network_reset);
			boost::throw_exception(ex);
		}

		// 得到request_method.
		std::string request_method = "GET";
		option_item::iterator val = opts.find("request_method");
		if (val != opts.end())
		{
			if (val->second == "GET" || val->second == "HEAD" || val->second == "POST")
				request_method = val->second;
			opts.erase(val);	// 删除处理过的选项.
		}

		// 得到Host信息.
		std::string host = m_url.to_string(url::host_component | url::port_component);
		val = opts.find("Host");
		if (val != opts.end())
		{
			host = val->second;	// 使用选项设置中的主机信息.
			opts.erase(val);	// 删除处理过的选项.
		}

		// 得到Accept信息.
		std::string accept = "*/*";
		val = opts.find("Accept");
		if (val != opts.end())
		{
			accept = val->second;
			opts.erase(val);	// 删除处理过的选项.
		}

		// 循环构造其它选项.
		std::string other_option_string;
		for (val = opts.begin(); val != opts.end(); val++)
		{
			other_option_string = val->first + ": " + val->second + "\r\n";
		}

		// 整合各选项到Http请求字符串中.
		std::string request_string;
		m_request.consume(m_request.size());
		std::ostream request_stream(&m_request);
		request_stream << request_method << " ";
		request_stream << m_url.to_string(url::path_component | url::query_component);
		request_stream << " HTTP/1.0\r\n";
		request_stream << "Host: " << host << "\r\n";
		request_stream << "Accept: " << accept << "\r\n";
		request_stream << other_option_string << "\r\n";

		// 发送请求.
		boost::asio::write(http_socket(), m_request, ec);
		if (ec)
		{
			// 发送请求失败!
			boost::throw_exception(boost::system::system_error(ec));
		}
	}

protected:
	boost::asio::io_service &m_io_service;			// io_service引用.
	tcp::socket m_socket;							// socket.

#ifdef HTTP_ENABLE_OPENSSL
	boost::asio::ssl::context m_ssl_context;		// SSL上下文.
	boost::asio::ssl::stream<
		boost::asio::ip::tcp::socket> m_ssl_socket;	// SSL连接.
#endif

	request_opts m_request_opts;					// 向http服务器请求的头信息.
	response_opts m_response_opts;					// http服务器返回的http头信息.
	std::string m_protocol;							// 协议类型(http/https).
	url m_url;										// 保存当前请求的url.
	bool m_keep_alive;								// 获得connection选项, 同时受m_response_opts影响.
	int m_status_code;								// http返回状态码.
	std::string m_content_type;						// 数据类型.
	std::size_t m_content_length;					// 数据内容长度.
	std::string m_location;							// 重定向的地址.
	boost::asio::streambuf m_request;				// 请求缓冲.
	boost::asio::streambuf m_response;				// 回复缓冲.
};

}

#include "detail/abi_suffix.hpp"

#endif // __HTTP_STREAM_HPP__
