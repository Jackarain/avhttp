#include <string>
#include <memory>
#include <iostream>
#include <stdio.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

using boost::asio::ip::tcp;

#define ENABLE_LOGGER
#define LOGGING_COMPRESS_LOGS
#include "logging.hpp"

template<class ... T> inline constexpr bool always_false = false;
#include "simple_http.hpp"

using namespace avhttp;

std::string fetch(const std::string& url, const std::string& proxy)
{
    std::string context;
    boost::asio::io_context io_context;
    boost::asio::spawn(io_context,
	   [&](boost::asio::yield_context yield) mutable
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
		   if (res.result() != boost::beast::http::status::ok) {
			   std::ostringstream oss;
			   oss << res << ", error: " << ec.message();
			   if (proxy.empty())
				   LOG_DBG << "打开Url '" << url << "' 失败" << oss.str();
			   else
				   LOG_DBG << "通过代理 '" << proxy << "' 打开Url '" << url << "' 失败" << oss.str();
			   return;
		   }
		   context = boost::beast::buffers_to_string(res.body().data());
		   LOG_DBG << "URL: '" << url << "', PROXY: '" << proxy << "', 获取到内容:\n" << context;
	   });
    io_context.run();
	return context;
}
