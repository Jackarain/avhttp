//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>

#include "url_parser.hpp"
#include "io.hpp"
#include "logging.hpp"
#include "handler_type_check.hpp"

namespace socks
{
	//////////////////////////////////////////////////////////////////////////

	class error_category_impl;

	template<class error_category>
	const boost::system::error_category& error_category_single()
	{
		static error_category error_category_instance;
		return reinterpret_cast<const boost::system::error_category&>(error_category_instance);
	}

	inline const boost::system::error_category& error_category()
	{
		return error_category_single<socks::error_category_impl>();
	}

	namespace errc {
		enum errc_t
		{
			/// SOCKS unsupported version.
			socks_unsupported_version = 1000,

			/// SOCKS username required.
			socks_username_required,

			/// SOCKS unsupported authentication version.
			socks_unsupported_authentication_version,

			/// SOCKS authentication error.
			socks_authentication_error,

			/// SOCKS general failure.
			socks_general_failure,

			/// SOCKS command not supported.
			socks_command_not_supported,

			/// SOCKS no identd running.
			socks_no_identd,

			/// SOCKS no identd running.
			socks_identd_error,

			/// request rejected or failed.
			socks_request_rejected_or_failed,

			/// request rejected becasue SOCKS server cannot connect to identd on the client.
			socks_request_rejected_cannot_connect,

			/// request rejected because the client program and identd report different user - ids
			socks_request_rejected_incorrect_userid,

			/// unknown error.
			socks_unknown_error,
		};

		inline boost::system::error_code make_error_code(errc_t e)
		{
			return boost::system::error_code(static_cast<int>(e), socks::error_category());
		}
	}

	class error_category_impl
		: public boost::system::error_category
	{
		virtual const char* name() const BOOST_SYSTEM_NOEXCEPT
		{
			return "SOCKS";
		}

		virtual std::string message(int e) const
		{
			switch (e)
			{
			case errc::socks_unsupported_version:
				return "SOCKS unsupported version";
			case errc::socks_username_required:
				return "SOCKS username required";
			case errc::socks_unsupported_authentication_version:
				return "SOCKS unsupported authentication version";
			case errc::socks_authentication_error:
				return "SOCKS authentication error";
			case errc::socks_general_failure:
				return "SOCKS general failure";
			case errc::socks_command_not_supported:
				return "SOCKS command not supported";
			case errc::socks_no_identd:
				return "SOCKS no identd running";
			case errc::socks_identd_error:
				return "SOCKS no identd running";
			case errc::socks_request_rejected_or_failed:
				return "SOCKS request rejected or failed";
			case errc::socks_request_rejected_cannot_connect:
				return "SOCKS request rejected becasue SOCKS server cannot connect to identd on the client";
			case errc::socks_request_rejected_incorrect_userid:
				return "SOCKS request rejected because the client program and identd report different user";
			case errc::socks_unknown_error:
				return "SOCKS unknown error";
			default:
				return "Unknown PROXY error";
			}
		}
	};
}

namespace boost {
	namespace system {

		template <>
		struct is_error_code_enum<socks::errc::errc_t>
		{
			static const bool value = true;
		};

	} // namespace system
} // namespace boost


namespace socks {

	using namespace endian;
	using namespace util;
	using boost::asio::ip::tcp;

	inline boost::system::error_code do_socks5(uri& parser,
		const std::string& target, const std::string& target_port,
		tcp::socket& socket, boost::asio::yield_context& yield)
	{
		std::size_t bytes_to_write = parser.username().empty() ? 3 : 4;
		boost::asio::streambuf request;
		boost::asio::mutable_buffer b = request.prepare(bytes_to_write);
		char* p = boost::asio::buffer_cast<char*>(b);

		write_uint8(5, p); // SOCKS VERSION 5.
		if (parser.username().empty())
		{
			write_uint8(1, p); // 1 authentication method (no auth)
			write_uint8(0, p); // no authentication
		}
		else
		{
			write_uint8(2, p); // 2 authentication methods
			write_uint8(0, p); // no authentication
			write_uint8(2, p); // username/password
		}

		request.commit(bytes_to_write);

		boost::system::error_code ec;
		boost::asio::async_write(socket, request, boost::asio::transfer_exactly(bytes_to_write), yield[ec]);
		if (ec)
			return ec;

		boost::asio::streambuf response;
		boost::asio::async_read(socket, response, boost::asio::transfer_exactly(2), yield[ec]);
		if (ec)
			return ec;

		int method;
		bool authed = false;

		{
			int version;

			boost::asio::const_buffer b1 = response.data();
			const char* p1 = boost::asio::buffer_cast<const char*>(b1);
			version = read_uint8(p1);
			method = read_uint8(p1);
			if (version != 5)	// 版本不等于5, 不支持socks5.
			{
				ec = socks::errc::socks_unsupported_version;
				return ec;
			}
		}

		if (method == 2)
		{
			if (parser.username().empty())
			{
				ec = socks::errc::socks_username_required;
				return ec;
			}

			// start sub-negotiation.
			request.consume(request.size());

			std::size_t bytes_to_write1 = parser.username().size() + parser.password().size() + 3;
			boost::asio::mutable_buffer mb = request.prepare(bytes_to_write1);
			char* mp = boost::asio::buffer_cast<char*>(mb);

			write_uint8(1, mp);
			write_uint8(static_cast<int8_t>(parser.username().size()), mp);
			write_string(std::string(parser.username()), mp);
			write_uint8(static_cast<int8_t>(parser.password().size()), mp);
			write_string(std::string(parser.password()), mp);
			request.commit(bytes_to_write1);

			// 发送用户密码信息.
			boost::asio::async_write(socket, request, boost::asio::transfer_exactly(bytes_to_write1), yield[ec]);
			if (ec)
				return ec;

			// 读取状态.
			response.consume(response.size());
			boost::asio::async_read(socket, response, boost::asio::transfer_exactly(2), yield[ec]);
			if (ec)
				return ec;

			// 读取版本状态.
			boost::asio::const_buffer cb = response.data();
			const char* cp = boost::asio::buffer_cast<const char*>(cb);

			int version = read_uint8(cp);
			int status = read_uint8(cp);

			// 不支持的认证版本.
			if (version != 1)
			{
				ec = errc::socks_unsupported_authentication_version;
				return ec;
			}

			// 认证错误.
			if (status != 0)
			{
				ec = errc::socks_authentication_error;
				return ec;
			}

			authed = true;
		}

		if (method == 0 || authed)
		{
			request.consume(request.size());
			std::size_t bytes_to_write1 = 7 + target.size();
			boost::asio::mutable_buffer mb = request.prepare(bytes_to_write1);
			char* wp = boost::asio::buffer_cast<char*>(mb);

			// 发送socks5连接命令.
			write_uint8(5, wp); // SOCKS VERSION 5.
			write_uint8(1, wp); // CONNECT command.
			write_uint8(0, wp); // reserved.
			write_uint8(3, wp); // address type. TODO: 这里根据使用是IP还是域名区别使用1或3
			BOOST_ASSERT(target.size() <= 255);
			write_uint8(static_cast<int8_t>(target.size()), wp);	// domainname size.
			std::copy(target.begin(), target.end(), wp);		// domainname.
			wp += target.size();
			write_uint16(static_cast<uint16_t>(atoi(target_port.c_str())), wp);	// port.

			request.commit(bytes_to_write1);
			boost::asio::async_write(socket, request, boost::asio::transfer_exactly(bytes_to_write1), yield[ec]);
			if (ec)
				return ec;

			std::size_t bytes_to_read = 10;
			response.consume(response.size());
			boost::asio::async_read(socket, response,
				boost::asio::transfer_exactly(bytes_to_read), yield[ec]);
			if (ec)
				return ec;

			boost::asio::const_buffer cb = response.data();
			const char* rp = boost::asio::buffer_cast<const char*>(cb);
			int version = read_uint8(rp);
			int resp = read_uint8(rp);
			read_uint8(rp);	// skip RSV.
			int atyp = read_uint8(rp);

			if (atyp == 1) // ADDR.PORT
			{
				tcp::endpoint remote_endp;
				remote_endp.address(boost::asio::ip::address_v4(read_uint32(rp)));
				remote_endp.port(read_uint16(rp));

				// LOG_DBG << "* SOCKS remote host: " << remote_endp.address().to_string() << ":" << remote_endp.port();
			}
			else if (atyp == 3) // DOMAIN
			{
				auto domain_length = read_uint8(rp);

				boost::asio::async_read(socket, response,
					boost::asio::transfer_exactly(domain_length - 3), yield[ec]);
				if (ec)
					return ec;

				rp = boost::asio::buffer_cast<const char*>(response.data()) + 5;

				std::string domain;
				for (int i = 0; i < domain_length; i++)
					domain.push_back(read_uint8(rp));
				auto port = read_uint16(rp);
				(void)port;
				// LOG_DBG << "* SOCKS remote host: " << domain << ":" << port;
			}
			else
			{
				ec = errc::socks_general_failure;
				return ec;
			}

			if (version != 5)
			{
				ec = errc::socks_unsupported_version;
				return ec;
			}

			if (resp != 0)
			{
				ec = errc::socks_general_failure;
				// 得到更详细的错误信息.
				switch (resp)
				{
				case 2: ec = boost::asio::error::no_permission; break;
				case 3: ec = boost::asio::error::network_unreachable; break;
				case 4: ec = boost::asio::error::host_unreachable; break;
				case 5: ec = boost::asio::error::connection_refused; break;
				case 6: ec = boost::asio::error::timed_out; break;
				case 7: ec = errc::socks_command_not_supported; break;
				case 8: ec = boost::asio::error::address_family_not_supported; break;
				}

				return ec;
			}

			ec = boost::system::error_code();	// 没有发生错误, 返回.
			return ec;
		}

		ec = boost::asio::error::address_family_not_supported;
		return ec;
	}

	enum
	{
		SOCKS_CMD_CONNECT = 1,
		SOCKS_VERSION_4 = 4,
		SOCKS4_REQUEST_GRANTED = 90,
		SOCKS4_REQUEST_REJECTED_OR_FAILED = 91,
		SOCKS4_CANNOT_CONNECT_TARGET_SERVER = 92,
		SOCKS4_REQUEST_REJECTED_USER_NO_ALLOW = 93,
	};

	inline boost::system::error_code do_socks4(uri& parser,
		const std::string& target, const std::string& target_port,
		tcp::socket& socket, boost::asio::yield_context& yield)
	{
		std::size_t bytes_to_write = parser.username().empty() ? 3 : 4;
		boost::asio::streambuf request;
		boost::asio::mutable_buffer b = request.prepare(bytes_to_write);
		char* p = boost::asio::buffer_cast<char*>(b);

		write_uint8(SOCKS_VERSION_4, p); // SOCKS VERSION 4.
		write_uint8(SOCKS_CMD_CONNECT, p); // CONNECT.
		write_uint16(static_cast<uint16_t>(atoi(target_port.c_str())), p); // DST PORT.

		boost::system::error_code ec;
		auto address = boost::asio::ip::address_v4::from_string(target, ec);
		if (ec)
			return ec;

		write_uint32(address.to_uint(), p); // DST IP.
		if (!parser.username().empty())
		{
			std::copy(parser.username().begin(), parser.username().end(), p);    // USERID
			p += parser.username().size();
		}

		write_uint8(0, p); // NULL.

		request.commit(bytes_to_write);
		boost::asio::async_write(socket, request, yield[ec]);
		if (ec)
			return ec;

		boost::asio::streambuf response;
		boost::asio::async_read(socket, response,
				boost::asio::transfer_exactly(8), yield[ec]);
		if (ec)
			return ec;

		auto resp = boost::asio::buffer_cast<const char*>(response.data());
		read_uint8(resp); // VN is the version of the reply code and should be 0.
		auto cd = read_uint8(resp);
		if (cd != SOCKS4_REQUEST_GRANTED)
		{
			switch (cd)
			{
			case SOCKS4_REQUEST_REJECTED_OR_FAILED:
				ec = socks::errc::socks_request_rejected_or_failed;
				break;
			case SOCKS4_CANNOT_CONNECT_TARGET_SERVER:
				ec = errc::socks_request_rejected_cannot_connect;
				break;
			case SOCKS4_REQUEST_REJECTED_USER_NO_ALLOW:
				ec = errc::socks_request_rejected_incorrect_userid;
				break;
			default:
				ec = errc::socks_unknown_error;
				break;
			}
		}

		return ec;
	}

	inline std::tuple<bool, boost::system::error_code> do_proxy(
		const std::string& url, tcp::socket& socket,
		const std::string& target, const std::string& target_port, boost::asio::yield_context& yield)
	{
		boost::system::error_code ec;
		uri parser;

		// Parser socks url.
		if (!parser.parse(url))
		{
			ec = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
			return {false, ec};
		}

		std::string port_string(parser.port());
		if (port_string.empty())
			port_string = "1080";

		// do socks5.
		if (parser.scheme() == "socks5")
		{
			ec = do_socks5(parser, target, target_port, socket, yield);
			return { !ec, ec };
		}

		// do socks4.
		if (parser.scheme() == "socks4")
		{
			ec = do_socks4(parser, target, target_port, socket, yield);
			return { !ec, ec };
		}

		ec = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return {false, ec};
	}

	template<class Handler>
	BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, bool))
	async_do_proxy(const std::string& socks_url,
		const std::string& address, const std::string& port, tcp::socket& socket, Handler&& handler)
	{
		AVHTTP_HANDLER_TYPE_CHECK(Handler, void(boost::system::error_code, bool));
		boost::asio::async_completion<Handler, void(boost::system::error_code, bool)> init(handler);

		boost::asio::spawn(socket.get_executor(),
			[url = socks_url, &socket, target = address, port = port, handler = init.completion_handler]
			(boost::asio::yield_context yield) mutable
		{
			boost::system::error_code ec;
			bool ret;
			std::tie(ret, ec) = do_proxy(url, socket, target, port, yield);

			auto executor = boost::asio::get_associated_executor(handler);
			boost::asio::post(executor, [ec, ret, handler]() mutable
			{
				handler(ec, ret);
			});
		});

		return init.result.get();
	}

}
