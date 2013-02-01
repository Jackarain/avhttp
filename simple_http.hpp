//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_local_shared.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/find.hpp>

#include <boost/variant.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/filesystem.hpp>

#include "http_last_modified.hpp"
#include "handler_type_check.hpp"
#include "url_parser.hpp"
#include "socks_proxy.hpp"
#include "default_cert.hpp"
#include "logging.hpp"
#include "async_connect.hpp"

#ifndef AVHTTP_VERSION_STRING
#  define AVHTTP_VERSION_STRING         "avhttp/1.0"
#endif

#ifdef AVHTTP_USE_FLAT_BUFFER
#  define AVHTTP_RECEIVE_BODY_MAX       (200 * 1024 * 1024)
#  define AVHTTP_RECEIVE_BUFFER_SIZE    (5 * 1024 * 1024)
#endif

namespace avhttp
{
	namespace beast = boost::beast;         // from <boost/beast.hpp>
	namespace http = beast::http;           // from <boost/beast/http.hpp>
	namespace net = boost::asio;            // from <boost/asio.hpp>
	using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
	using uri = util::uri;
	using http_request = http::request<http::string_body>;
	using http_response = http::response<http::dynamic_body>;

	inline std::string response_body_string(const http_response& res)
	{
		return beast::buffers_to_string(res.body().data());
	}

	/*
	使用示例:

	simple_http s(executor);
	http_request req{ boost::beast::http::verb::get, "/", 11 };
	req.set(boost::beast::http::field::host, "www.baidu.com");
	req.set(boost::beast::http::field::user_agent, AVHTTP_VERSION_STRING);
	s.async_perform("https://www.baidu.com", req, [](boost::system::error_code ec, http_response res) {
		if (!ec)
		{
			if (res.result() != boost::beast::http::status::ok)
				std::cout << res << std::endl;
			else
				std::cout << boost::beast::buffers_to_string(res.body().data()) << std::endl;
		}
		else
		{
			std::cout << ec.message() << std::endl;
		}
	});

	*/

	static const std::string chrome_user_agent = R"(Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36)";
	static const std::string edge_user_agent = R"(Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/64.0.3282.140 Safari/537.36 Edge/18.17763)";
	static const std::string ie_user_agent = R"(Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko)";
	static const std::string curl_user_agent = R"(curl/7.64.0)";

	template <typename Executor = boost::asio::io_context::executor_type>
	class basic_simple_http
	{
		using ssl_stream = beast::ssl_stream<beast::tcp_stream>;
		using ssl_stream_ptr = boost::local_shared_ptr<ssl_stream>;
		using tcp_stream = beast::tcp_stream;
		using tcp_stream_ptr = boost::local_shared_ptr<tcp_stream>;
		using variant_socket = boost::variant<tcp_stream_ptr, ssl_stream_ptr>;
		using download_handler = std::function<void(const void*, std::size_t)>;

	public:
		using executor_type = Executor;

		basic_simple_http(const executor_type& executor, bool check_cert = true)
			: m_executor(executor)
			, m_check_certificate(check_cert)
		{}
		~basic_simple_http() = default;

	public:

		// 检查和设置证书认证是否启用.
		bool check_certificate() const
		{
			return m_check_certificate;
		}

		void check_certificate(bool check)
		{
			m_check_certificate = check;
		}

		// 加载证书文件路径或证书文件，证书数据.
		void load_certificate_path(const std::string& path)
		{
			m_cert_path = path;
		}

		void load_certificate_file(const std::string& path)
		{
			m_cert_file = path;
		}

		void load_root_certificates(const std::string& data)
		{
			m_cert_data = data;
		}

		// 保存到文件.
		void dump(const std::string& file)
		{
			m_dump_file = file;
		}

		// 下载百分比.
		std::optional<double> percent()
		{
			return m_download_percent;
		}

		// 下载剩余大小, 如果服务器提供了内容长度.
		std::optional<std::size_t> content_length_remaining()
		{
			return m_content_lentgh_remaining;
		}

		// 下载内容大小, 如果服务器提供了内容长度.
		std::optional<std::size_t> content_length()
		{
			return m_content_lentgh;
		}

		// 设置下载数据回调.
		void download_cb(download_handler cb)
		{
			m_download_handler = cb;
		}

		// 重置, 用于重复使用应该对象.
		void reset()
		{
			m_stream = variant_socket{};
			m_dump_file.clear();
			m_download_percent = {};
			m_content_lentgh = {};
			m_content_lentgh_remaining = {};
			m_download_handler = download_handler{};
			m_url.clear();
		}

		executor_type get_executor() noexcept
		{
			return m_executor;
		}

		struct initiate_do_perform
		{
			initiate_do_perform(basic_simple_http* p)
				: m_simple_http(p)
			{}

			template <typename Handler>
			void operator()(Handler&& handler, const std::string& url, const std::string& socks, http_request* req) const
			{
				boost::asio::detail::non_const_lvalue<Handler> handler2(handler);
				auto shttp = m_simple_http;

				net::spawn(shttp->m_executor, [shttp, url, socks, req, handler = std::move(handler2.value)]
				(net::yield_context yield) mutable
				{
					http_response result;
					boost::system::error_code ec;
					ec = shttp->do_async_perform(yield, *req, result, url, socks);

					auto executor = boost::asio::get_associated_executor(handler);
					boost::asio::post(executor, [ec, result = std::move(result), handler]() mutable
					{
						handler(ec, result);
					});
				}, boost::coroutines::attributes(5 * 1024 * 1024));
			}

			basic_simple_http* m_simple_http;
		};

		// 异步执行一个url请求, 请求参数由req指定, 请求返回通过error_code 或 http_response
		// 得到, 如果发生错误, 则会得到error_code, 若请求正常则返回http_response.
		template<class Handler>
		BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, http_response))
		async_perform(const std::string& url, http_request& req, Handler&& handler)
		{
			AVHTTP_HANDLER_TYPE_CHECK(Handler, void(boost::system::error_code, http_response));

			return boost::asio::async_initiate<Handler,
				void(boost::system::error_code, http_response)>(initiate_do_perform(this), handler, url, "", &req);
		}

		// 异步执行一个url请求, 请求参数由req指定, 请求返回通过error_code 或 http_response
		// 得到, 如果发生错误, 则会得到error_code, 若请求正常则返回http_response.
		// 可指定sock5 proxy, 如 socks5://127.0.0.1:1080
		template<class Handler>
		BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, http_response))
		async_perform(const std::string& url, const std::string& socks, http_request& req, Handler&& handler)
		{
			AVHTTP_HANDLER_TYPE_CHECK(Handler, void(boost::system::error_code, http_response));

			return boost::asio::async_initiate<Handler,
				void(boost::system::error_code, http_response)>(initiate_do_perform(this), handler, url, socks, &req);
		}

		// 关闭http底层调用, 强制返回.
		void close()
		{
			boost::apply_visitor([](auto p) mutable
			{
				boost::system::error_code ec;
				if (p)
				{
					auto& s = beast::get_lowest_layer(*p);
					s.socket().close(ec);
				}
			}, m_stream);
		}

	private:
		boost::system::error_code do_async_perform(net::yield_context yield,
			http_request& req, http_response& result, const std::string& url, std::string socks_url = "")
		{
			boost::system::error_code ec;
			uri parser;

			// Parser url.
			if (!parser.parse(url))
			{
				ec = boost::asio::error::make_error_code(net::error::invalid_argument);
				return ec;
			}

			m_url = url;
			std::string host(parser.host());

			// check request params.
			auto host_it = req.find(boost::beast::http::field::host);
			if (host_it == req.end())
				req.set(boost::beast::http::field::host, host);

			if (req.method() == boost::beast::http::verb::unknown)
				req.method(boost::beast::http::verb::get);

			auto user_agent_it = req.find(boost::beast::http::field::user_agent);
			if (user_agent_it == req.end())
				req.set(boost::beast::http::field::user_agent, AVHTTP_VERSION_STRING);

			if (req.target() == "")
			{
				std::string query;
				if (parser.query() != "")
				{
					auto q = std::string(parser.query());
					if (q[0] == '?')
						query = std::string(parser.query());
					else
						query = "?" + std::string(parser.query());
				}

				if (std::string(parser.path()) == "")
					req.target("/" + query);
				else
					req.target(std::string(parser.path()) + query);
			}
			if (req.method() == http::verb::post)
			{
				if (!req.has_content_length() && !req.body().empty())
					req.content_length(req.body().size());
			}

			if (boost::to_lower_copy(std::string(parser.scheme())) == "https")
			{
				m_ssl_ctx = boost::make_local_shared<net::ssl::context>(net::ssl::context::sslv23_client);

				if (m_check_certificate)
				{
					bool load_cert = false;
					m_ssl_ctx->set_verify_mode(net::ssl::verify_peer);

					const char *dir;
					dir = getenv(X509_get_default_cert_dir_env());
					if (!dir)
						dir = X509_get_default_cert_dir();
					if (boost::filesystem::exists(dir))
					{
						m_ssl_ctx->add_verify_path(dir, ec);
						if (ec)
							LOG_WARN << "add_verify_path fail, check your cert dir: " << dir;
					}

					if (!m_cert_path.empty())
					{
						load_cert = true;
						m_ssl_ctx->add_verify_path(m_cert_path, ec);
						if (ec)
							return ec;
					}
					if (!m_cert_file.empty())
					{
						load_cert = true;
						m_ssl_ctx->load_verify_file(m_cert_file, ec);
						if (ec)
							return ec;
					}
					if (!m_cert_data.empty())
					{
						load_cert = true;
						m_ssl_ctx->add_certificate_authority(net::buffer(m_cert_data.data(), m_cert_data.size()), ec);
						if (ec)
							LOG_WARN << "add_certificate_authority fail, check your cert data!";
					}
					if (!load_cert)
					{
						auto cert = default_root_certificates();
						m_ssl_ctx->add_certificate_authority(net::buffer(cert.data(), cert.size()), ec);
						if (ec)
							return ec;
					}

					m_ssl_ctx->set_verify_callback(
						boost::asio::ssl::rfc2818_verification(host), ec);
					if (ec)
						return ec;
				}

				variant_socket newsocket(boost::make_local_shared<ssl_stream>(m_executor, *m_ssl_ctx));
				m_stream.swap(newsocket);

				auto& stream = *(boost::get<ssl_stream_ptr>(m_stream));

				// Set SNI Hostname (many hosts need this to handshake successfully)
				if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
				{
					ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
					return ec;
				}

				tcp::resolver resolver(m_executor);
				std::string port(parser.port());
				if (port.empty())
					port = "443";

				// Look up the domain name
				auto const results = resolver.async_resolve(host, port, yield[ec]);
				if (ec)
					return ec;

				// Set the timeout.
				beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

				if (!socks_url.empty())
				{
					uri socks_parser;

					// Parser socks url.
					if (!socks_parser.parse(socks_url))
					{
						ec = boost::asio::error::make_error_code(net::error::invalid_argument);
						return ec;
					}

					std::string socks_host(socks_parser.host());
					std::string socks_port(socks_parser.port());
					if (socks_port.empty())
						socks_port = "1080";

					auto const socks_results = resolver.async_resolve(socks_host, socks_port, yield[ec]);
					if (ec)
						return ec;

					asio_util::async_connect(get_lowest_layer(stream).socket(), socks_results, yield[ec]);
					if (ec)
						return ec;

					auto& socket = get_lowest_layer(stream).socket();
					socks::async_do_proxy(socks_url, host, port, socket, yield[ec]);
					if (ec)
						return ec;

					boost::asio::socket_base::keep_alive option(true);
					socket.set_option(option, ec);
				}
				else
				{
					// Make the connection on the IP address we get from a lookup
					asio_util::async_connect(get_lowest_layer(stream).socket(), results, yield[ec]);
					if (ec)
						return ec;

					boost::asio::socket_base::keep_alive option(true);
					get_lowest_layer(stream).socket().set_option(option, ec);
				}

				// Perform the SSL handshake
				stream.async_handshake(net::ssl::stream_base::client, yield[ec]);
				if (ec)
					return ec;

				// Set the timeout.
				beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
			}
			else if (boost::to_lower_copy(std::string(parser.scheme())) == "http")
			{
				// These objects perform our I/O
				tcp::resolver resolver(m_executor);

				variant_socket newsocket(boost::make_local_shared<tcp_stream>(m_executor));
				m_stream.swap(newsocket);

				auto& stream = *(boost::get<tcp_stream_ptr>(m_stream));
				std::string port(parser.port());
				if (port.empty())
					port = "80";

				// Look up the domain name
				auto const results = resolver.async_resolve(host, port, yield[ec]);
				if (ec)
					return ec;

				// Set the timeout.
				beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

				if (!socks_url.empty())
				{
					uri socks_parser;

					// Parser socks url.
					if (!socks_parser.parse(socks_url))
					{
						ec = boost::asio::error::make_error_code(net::error::invalid_argument);
						return ec;
					}

					std::string socks_host(socks_parser.host());
					std::string socks_port(socks_parser.port());
					if (socks_port.empty())
						socks_port = "1080";

					auto const socks_results = resolver.async_resolve(socks_host, socks_port, yield[ec]);
					if (ec)
						return ec;

					asio_util::async_connect(stream.socket(), socks_results, yield[ec]);
					if (ec)
						return ec;

					boost::asio::socket_base::keep_alive option(true);
					stream.socket().set_option(option, ec);

					auto& socket = stream.socket();
					socks::async_do_proxy(socks_url, host, port, socket, yield[ec]);
					if (ec)
						return ec;
				}
				else
				{
					// Make the connection on the IP address we get from a lookup
					asio_util::async_connect(stream.socket(), results, yield[ec]);
					if (ec)
						return ec;

					boost::asio::socket_base::keep_alive option(true);
					stream.socket().set_option(option, ec);
				}

				// Set the timeout.
				beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
			}
			else
			{
				BOOST_ASSERT(false && "not supported scheme!");
				ec = boost::asio::error::make_error_code(net::error::invalid_argument);
				return ec;
			}

			ec = boost::apply_visitor([&](auto p) mutable
					{ return do_perform(*p, req, result, yield); }
				, m_stream);

			return ec;
		}

		template<class S, class B>
		boost::system::error_code async_read_http(S& stream, B& buffer, http_response& msg, net::yield_context yield)
		{
			boost::beast::http::response_parser<http_response::body_type> p{std::move(msg)};
			boost::system::error_code ec;

			p.body_limit(std::numeric_limits<std::size_t>::max());
			std::size_t total = 0;

			boost::filesystem::ofstream file;
			boost::filesystem::path dumppath(m_dump_file);
			bool dump_to_file = false;

			if (!m_dump_file.empty())
			{
				// create file or directorys.
				if (dumppath.has_parent_path() && !boost::filesystem::exists(dumppath.parent_path()))
				{
					boost::system::error_code ignore_ec;
					boost::filesystem::create_directories(dumppath.parent_path(), ignore_ec);
				}
			}

			do {
				auto bytes = http::async_read_some(stream, buffer, p, yield[ec]);
				boost::ignore_unused(bytes);
				if (ec)
					return ec;

				auto& body = p.get().body();
				auto bodysize = body.size();

				total += bodysize;

				auto length = p.content_length();
				if (length)
				{
					m_content_lentgh = *length;
					m_content_lentgh_remaining = *p.content_length_remaining();

					m_download_percent = 1.0 - (*m_content_lentgh_remaining / static_cast<double>(*length));
				}

				if (p.is_header_done() && !dump_to_file)
				{
					if (!m_dump_file.empty() && p.get().result() == boost::beast::http::status::ok)
					{
						{
							boost::system::error_code ignore_ec;
							boost::filesystem::remove(dumppath, ignore_ec);
						}

						file.open(dumppath, std::ios_base::binary | std::ios_base::trunc);
						if (file.good())
							dump_to_file = true;
					}
				}

				if ((m_download_handler || dump_to_file) && bodysize > 0)
				{
					for (auto const buf : boost::beast::buffers_range(body.data()))
					{
						auto bufsize = buf.size();
						auto bufptr = static_cast<const char*>(buf.data());

						if (dump_to_file)
							file.write(bufptr, bufsize);

						if (m_download_handler)
							m_download_handler(bufptr, bufsize);
					}

					body.consume(body.size());
				}
			} while (!p.is_done());

			// Transfer ownership of the message container in the parser to the caller.
			msg = p.release();

			// Last write time.
			if (dump_to_file)
			{
				file.close();

				auto lm = msg.find(boost::beast::http::field::last_modified);
				if (lm != msg.end())
				{
					boost::system::error_code ignore_ec;
					auto tm = http_parse_last_modified(lm->value().to_string());
					if (tm != -1)
						boost::filesystem::last_write_time(dumppath, tm, ignore_ec);
				}
			}

			return ec;
		}

		template<class S>
		boost::system::error_code do_perform(S& stream, http_request& req, http_response& result, net::yield_context yield)
		{
			boost::system::error_code ec;

			// Send the HTTP request to the remote host
			http::async_write(stream, req, yield[ec]);
			if (ec)
				return ec;

			// This buffer is used for reading and must be persisted
#ifdef AVHTTP_USE_FLAT_BUFFER
			beast::flat_buffer b(AVHTTP_RECEIVE_BODY_MAX);
			b.reserve(AVHTTP_RECEIVE_BUFFER_SIZE);
#else
			beast::multi_buffer b;
#endif

			// Receive the HTTP response
			ec = async_read_http(stream, b, result, yield);
			if (ec)
				return ec;

			// Gracefully close the socket
			if constexpr (std::is_same_v<std::decay_t<S>, ssl_stream>)
			{
				beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
				boost::system::error_code ignore_ec;
				stream.async_shutdown(yield[ignore_ec]);
			}
			else if constexpr (std::is_same_v<std::decay_t<S>, tcp_stream>)
			{
				stream.socket().shutdown(tcp::socket::shutdown_both, ec);
			}
			else
			{
				static_assert(always_false<S>, "non-exhaustive visitor!");
			}

			return ec;
		}

	private:
		executor_type m_executor;
		variant_socket m_stream;
		boost::local_shared_ptr<net::ssl::context> m_ssl_ctx;
		bool m_check_certificate;
		std::string m_cert_path;
		std::string m_cert_file;
		std::string m_cert_data;
		std::string m_dump_file;
		std::optional<double> m_download_percent;
		std::optional<std::size_t> m_content_lentgh;
		std::optional<std::size_t> m_content_lentgh_remaining;
		download_handler m_download_handler;
		std::string m_url;
	};

	using simple_http = basic_simple_http<>;
}
