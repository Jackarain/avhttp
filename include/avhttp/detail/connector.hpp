
#pragma once

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "coro/yield.hpp"

namespace boost {

namespace detail{

struct my_connect_condition{
	template <typename Iterator>
	Iterator operator()(
		const boost::system::error_code& ec,
		Iterator next)
	{
// 		if (ec) std::cout << "Error: " << ec.message() << std::endl;
// 		std::cout << "Trying: " << next->endpoint() << std::endl;
		return next;
	}
};

}
	
class async_connect : coro::coroutine
{
public:
	typedef void result_type; // for boost::bind
public:
	template<class Socket, class Handler>
	async_connect(Socket & socket,const typename Socket::protocol_type::resolver::query & _query, BOOST_ASIO_MOVE_ARG(Handler) _handler)
		:handler(_handler),io_service(socket.get_io_service())
	{
		BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;
		
		typedef typename Socket::protocol_type::resolver resolver_type;

		boost::shared_ptr<resolver_type>
			resolver(new resolver_type(io_service));
		resolver->async_resolve(_query, boost::bind(*this, _1, _2, boost::ref(socket), resolver));
	}

	template<class Socket>
	void operator()(const boost::system::error_code & ec, typename Socket::protocol_type::resolver::iterator begin, Socket & socket, boost::shared_ptr<typename Socket::protocol_type::resolver> resolver)
	{
		reenter(this)
		{
			_yield boost::asio::async_connect(socket,begin,detail::my_connect_condition(),boost::bind(*this, _1, _2, boost::ref(socket), resolver));
			io_service.post(asio::detail::bind_handler(handler,ec));
		}
	}

private:
	boost::asio::io_service&			io_service;
	boost::function<void(const boost::system::error_code & ec)>	handler;
};

}