#include <string>
#include <memory>
#include <iostream>
#include <stdio.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

#include <boost/boost/throw_exception.hpp>

using boost::asio::ip::tcp;

#define ENABLE_LOGGER
#define LOGGING_COMPRESS_LOGS
#include "logging.hpp"

template<class ... T> inline constexpr bool always_false = false;
#include "simple_http.hpp"

namespace avhttp {
	http_response fetch(const std::string& url, const std::string& proxy, boost::system::error_code& ec)
	{
		http_response res;

		{
			boost::asio::io_context io_context;
			boost::asio::spawn(io_context,
				[&](boost::asio::yield_context yield) mutable
				{
					simple_http http{ io_context.get_executor() };
					http_request req;
					req.set(boost::beast::http::field::user_agent, AVHTTP_VERSION_STRING);

					boost::system::error_code ec;
					if (proxy.empty())
						res = http.async_perform(url, req, yield[ec]);
					else
						res = http.async_perform(url, proxy, req, yield[ec]);
				});
			io_context.run();
		}

		return res;
	}

	http_response fetch(const std::string& url, const std::string& proxy)
	{
		boost::system::error_code ec;

		auto res = fetch(url, proxy, ec);
		if (ec)
			boost::throw_exception(boost::system::system_error(ec));

		return res;
	}

}
