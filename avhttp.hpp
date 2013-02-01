//
// avhttp.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2019 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <string>

#include <boost/system.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include "simple_http.hpp"

namespace avhttp {

	// 同步执行一个url请求, 请求返回http_response
	// 如果发生错误, 则会得到error_code.
	// 可指定sock5 proxy, 如 socks5://127.0.0.1:1080
	http_response fetch(const std::string& url, const std::string& proxy, boost::system::error_code& ec);

	// 同步执行一个url请求, 请求返回http_response
	// 如果发生错误, 则会抛出一个异常.
	// 可指定sock5 proxy, 如 socks5://127.0.0.1:1080
	http_response fetch(const std::string& url, const std::string& proxy);

	// 异步执行一个url请求, 请求参数由req指定, 请求返回通过error_code 或 http_response
	// 得到, 如果发生错误, 则会得到error_code, 若请求正常则返回http_response.
	// 可指定sock5 proxy, 如 socks5://127.0.0.1:1080
	template<class Handler>
	BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, http_response))
		async_fetch(boost::asio::io_context& io_context,
			const std::string& url, const std::string& proxy, BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		auto initiate_do_async_fetch = [](auto&& handler,
			boost::asio::io_context& io_context, const std::string& url, const std::string& proxy)
		{
			boost::asio::spawn(io_context,
				[&io_context, url, proxy, handler = std::move(handler)](boost::asio::yield_context yield) mutable
			{
				simple_http http{ io_context.get_executor() };
				http_request req;
				req.set(boost::beast::http::field::user_agent, AVHTTP_VERSION_STRING);
				http_response res;
				boost::system::error_code ec;
				if (proxy.empty())
					res = http.async_perform(url, req, yield[ec]);
				else
					res = http.async_perform(url, proxy, req, yield[ec]);
				handler(ec, res);
			});
		};

		return boost::asio::async_initiate<Handler, void(boost::system::error_code, http_response)>
			(initiate_do_async_fetch, handler, io_context, url, proxy);
	}
}
