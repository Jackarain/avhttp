//
// async_connect.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2019 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio/dispatch.hpp>
#include <boost/asio/connect.hpp>

#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_local_shared.hpp>

#include <atomic>
#include <utility>
#include <memory>
#include <vector>
#include <type_traits>


namespace asio_util {

	namespace detail {
		template <typename Stream>
		struct connect_params_detaila
		{
			std::atomic_int flag_;
			std::atomic_int num_;
			std::vector<boost::local_shared_ptr<Stream>> socket_;
		};

		template <typename Handler, typename ResultType>
		void do_result(Handler&& h, const boost::system::error_code& error, ResultType&& result)
		{
			h(error, result);
		}

		struct initiate_do_happy_eyeballs_connect
		{
			bool use_happy_eyeball = false;

			template <typename Stream, typename Handler, typename Iterator, typename ResultType = void>
			void do_async_connect(Handler&& handler, Stream& s, Iterator begin, Iterator end)
			{
				auto params = boost::make_local_shared<connect_params_detaila<Stream>>();

				params->flag_ = false;
				params->num_ = std::distance(begin, end);
				if (params->num_ == 0)
				{
					{
						boost::system::error_code error = boost::asio::error::no_data;
						auto h = std::move(handler);

						if constexpr (std::is_same_v<ResultType, typename Stream::endpoint_type>)
							do_result(h, error, *begin);
						if constexpr (!std::is_same_v<ResultType, typename Stream::endpoint_type>)
							do_result(h, error, begin);
					}

					return;
				}

				// happy eyeballs detection
				{
					bool has_a = false, has_aaaa = false;

					for (auto begin_ = begin; begin_ != end; begin_++)
					{
						has_aaaa |= boost::asio::ip::address(begin_->endpoint().address()).is_v6();
						has_a |= boost::asio::ip::address(begin_->endpoint().address()).is_v4();
					}

					if (has_aaaa && has_a)
						use_happy_eyeball = true;
				}

				std::vector<std::pair<std::function<void()>, bool>> connectors;

				for (; begin != end; begin++)
				{
					auto sock = boost::make_local_shared<Stream>(s.get_executor());
					params->socket_.emplace_back(sock);

					auto func = [begin, &s, params, sock, handler]() {
						sock->async_connect(*begin, [&s, params, begin, sock, handler]
						(const boost::system::error_code& error) mutable
						{
							if (!error)
							{
								if (params->flag_.exchange(true))
									return;

								s = std::move(*sock);
							}

							params->num_--;
							bool is_last = params->num_ == 0;

							if (error)
							{
								if (params->flag_ || !is_last)
									return;
							}

							auto& sockets = params->socket_;
							for (auto& t : sockets)
							{
								if (!t)
									continue;
								boost::system::error_code ignore_ec;
								t->cancel(ignore_ec);
							}

							auto executor = boost::asio::get_associated_executor(handler);
							auto h = std::move(handler);

							boost::asio::dispatch(executor, [error, h, begin]() mutable
							{
								if constexpr (std::is_same_v<ResultType, typename Stream::endpoint_type>)
									do_result(h, error, *begin);
								if constexpr (!std::is_same_v<ResultType, typename Stream::endpoint_type>)
									do_result(h, error, begin);
							});
						});
					};

					connectors.emplace_back(std::make_pair(func, begin->endpoint().address().is_v4()));
				}

				for (auto conn : connectors)
				{
					if (use_happy_eyeball && conn.second) // conn.second is v4.
					{
						using namespace std::chrono_literals;
						using boost::asio::steady_timer;

						// ipv4 delay 200ms.
						auto delay_timer = boost::make_local_shared<steady_timer>(s.get_executor());
						auto& timer = *delay_timer;

						timer.expires_from_now(200ms);
						timer.async_wait([delay_timer, conn, params] (const boost::system::error_code& error)
						{
							boost::ignore_unused(error);
							if (params->flag_)
								return;
							conn.first();
						});
					}
					else
					{
						conn.first();
					}
				}
			}

			template <typename Stream, typename Iterator, typename Handler>
			void operator()(Handler&& handler, Stream& s, Iterator begin, Iterator end)
			{
				do_async_connect(std::forward<Handler>(handler), s, begin, end);
			}

			template <typename Stream, typename EndpointSequence, typename Handler>
			void operator()(Handler&& handler, Stream& s, const EndpointSequence& endpoints)
			{
				auto begin = endpoints.begin();
				auto end = endpoints.end();
				using Iterator = decltype(begin);

				do_async_connect<Stream, Handler, Iterator, typename Stream::endpoint_type>(std::forward<Handler>(handler), s, begin, end);
			}
		};
	}

	template <typename Stream,
	typename Iterator, typename IteratorConnectHandler>
	BOOST_ASIO_INITFN_RESULT_TYPE(IteratorConnectHandler,
		void(boost::system::error_code, Iterator))
	async_connect(Stream& s, Iterator begin,
		BOOST_ASIO_MOVE_ARG(IteratorConnectHandler) handler,
		typename boost::asio::enable_if<!boost::asio::is_endpoint_sequence<Iterator>::value>::type* = 0)
	{
		return boost::asio::async_initiate<IteratorConnectHandler,
			void(boost::system::error_code, Iterator)>
			(detail::initiate_do_happy_eyeballs_connect{}, handler, s, begin, Iterator());
	}

	template <typename Stream,
		typename Iterator, typename IteratorConnectHandler>
		inline BOOST_ASIO_INITFN_RESULT_TYPE(IteratorConnectHandler,
			void(boost::system::error_code, Iterator))
		async_connect(Stream& s, Iterator begin, Iterator end,
			BOOST_ASIO_MOVE_ARG(IteratorConnectHandler) handler)
	{
		return boost::asio::async_initiate<IteratorConnectHandler,
			void(boost::system::error_code, Iterator)>
			(detail::initiate_do_happy_eyeballs_connect{}, handler, s, begin, end);
	}

	template <typename Stream,
		typename EndpointSequence, typename IteratorConnectHandler>
		inline BOOST_ASIO_INITFN_RESULT_TYPE(IteratorConnectHandler,
			void(boost::system::error_code, typename Stream::endpoint_type))
		async_connect(Stream& s, const EndpointSequence& endpoints,
			BOOST_ASIO_MOVE_ARG(IteratorConnectHandler) handler, 
			typename boost::asio::enable_if<boost::asio::is_endpoint_sequence<EndpointSequence>::value>::type* = 0)
	{
		return boost::asio::async_initiate<IteratorConnectHandler,
			void(boost::system::error_code, typename Stream::endpoint_type)>
			(detail::initiate_do_happy_eyeballs_connect{}, handler, s, endpoints);
	}
}
