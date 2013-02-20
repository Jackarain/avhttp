//
// multi_download.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MULTI_DOWNLOAD_HPP__
#define MULTI_DOWNLOAD_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <vector>
#include <list>

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>

#include "storage_interface.hpp"
#include "file.hpp"
#include "http_stream.hpp"
#include "rangefield.hpp"

namespace avhttp
{

// 下载模式.
enum downlad_mode
{
	// 紧凑模式下载, 紧凑模式是指, 将文件分片后, 从文件头开始, 一片紧接着一片,
	// 连续不断的下载.
	compact_mode,

	// 松散模式下载, 是指将文件分片后, 按连接数平分为N大块进行下载.
	dispersion_mode,

	// 快速读取模式下载, 这个模式是根据用户读取数据位置开始下载数据, 是尽快响应
	// 下载用户需要的数据.
	quick_read_mode
};

static const int default_request_piece_num = 10;
static const int default_time_out = 11;
static const int default_piece_size = 32768;
static const int default_connections_limit = 5;

// 下载设置.
struct settings
{
	settings ()
		: m_download_rate_limit(-1)
		, m_connections_limit(default_connections_limit)
		, m_piece_size(default_piece_size)
		, m_time_out(default_time_out)
		, m_request_piece_num(default_request_piece_num)
		, m_downlad_mode(dispersion_mode)
	{}

	int m_download_rate_limit;			// 下载速率限制, -1为无限制.
	int m_connections_limit;			// 连接数限制, -1为默认.
	int m_piece_size;					// 分块大小, 默认根据文件大小自动计算.
	int m_time_out;						// 超时断开, 默认为11秒.
	int m_request_piece_num;			// 每次请求的分片数, 默认为10.
	downlad_mode m_downlad_mode;		// 下载模式, 默认为dispersion_mode.
	fs::path m_meta_file;				// meta_file路径, 默认为当前路径下同文件名的.meta文件.
};

// 重定义http_stream_ptr指针.
typedef boost::shared_ptr<http_stream> http_stream_ptr;

// 定义http_stream_obj.
struct http_stream_object
{
	http_stream_object()
		: m_request_range(0, 0)
		, m_bytes_transferred(0)
		, m_bytes_downloaded(0)
		, m_done(false)
		, m_direct_reconnect(false)
	{}

	// http_stream对象.
	http_stream_ptr m_stream;

	// 数据缓冲, 下载时的缓冲.
	boost::array<char, 2048> m_buffer;

	// 请求的数据范围, 每次由multi_download分配一个下载范围, m_stream按这个范围去下载.
	range m_request_range;

	// 本次请求已经下载的数据, 相对于m_request_range, 当一个m_request_range下载完成后,
	// m_bytes_transferred自动置为0.
	boost::int64_t m_bytes_transferred;

	// 当前对象下载的数据统计.
	boost::int64_t m_bytes_downloaded;

	// 最后请求的时间.
	boost::posix_time::ptime m_last_request_time;

	// 是否操作功能完成.
	bool m_done;

	// 立即重新尝试连接.
	bool m_direct_reconnect;
};

// 重定义http_object_ptr指针.
typedef boost::shared_ptr<http_stream_object> http_object_ptr;


// multi_download类的具体实现.
class multi_download : public boost::noncopyable
{
public:
	multi_download(boost::asio::io_service &io)
		: m_io_service(io)
		, m_accept_multi(false)
		, m_keep_alive(false)
		, m_file_size(-1)
		, m_timer(io, boost::posix_time::seconds(0))
		, m_abort(false)
	{}
	~multi_download()
	{}

public:

	///打开multi_download开始下载.
	// @param u指定的url.
	// @param ec当发生错误时, 包含详细的错误信息.
	// @备注: 直接使用内部的file.hpp下载数据到文件, 若想自己指定数据下载到指定的地方
	// 可以通过调用另一个open来完成, 具体见另一个open的详细说明.
	void open(const url &u, boost::system::error_code &ec)
	{
		settings s;
		ec = open(u, s);
	}

	///打开multi_download开始下载, 打开失败抛出一个异常.
	// @param u指定的url.
	// @备注: 直接使用内部的file.hpp下载数据到文件, 若想自己指定数据下载到指定的地方
	// 可以通过调用另一个open来完成, 具体见另一个open的详细说明.
	void open(const url &u)
	{
		settings s;
		boost::system::error_code ec;
		ec = open(u, s);
		if (ec)
		{
			boost::throw_exception(boost::system::system_error(ec));
		}
	}

	///打开multi_download开始下载.
	// @param u指定的url.
	// @param s指定的设置信息.
	// @param p指定的storage创建函数指针, 默认为default_storage_constructor.
	// @返回error_code, 包含详细的错误信息.
	// @备注: 如果需要自定义multi_download存储数据方式, 需要按照storage_interface.hpp中的storage_interface
	// 虚接口来实现, 并实现一个storage_constructor_type函数用于创建你实现的存储接口, 通过这个open函数的参数3传入.
	boost::system::error_code open(const url &u, const settings &s, storage_constructor_type p = NULL)
	{
		boost::system::error_code ec;
		// 清空所有连接.
		m_streams.clear();
		m_file_size = -1;

		// 创建一个http_stream对象.
		http_object_ptr obj(new http_stream_object);

		request_opts req_opt;
		req_opt.insert("Range", "bytes=0-");
		req_opt.insert("Connection", "keep-alive");

		// 创建http_stream并同步打开, 检查返回状态码是否为206, 如果非206则表示该http服务器不支持多点下载.
		obj->m_stream.reset(new http_stream(m_io_service));
		http_stream &h = *obj->m_stream;
		h.request_options(req_opt);
		h.open(u, ec);
		// 打开失败则退出.
		if (ec)
		{
			return ec;
		}

		// 保存最终url信息.
		std::string location = h.location();
		if (!location.empty())
			m_final_url = location;
		else
			m_final_url = u;

		// 判断是否支持多点下载.
		std::string status_code;
		h.response_options().find("_status_code", status_code);
		if (status_code != "206")
			m_accept_multi = false;
		else
			m_accept_multi = true;

		// 得到文件大小.
		if (m_accept_multi)
		{
			std::string length;
			h.response_options().find("Content-Length", length);
			if (length.empty())
			{
				h.response_options().find("Content-Range", length);
				std::string::size_type f = length.find('/');
				if (f++ != std::string::npos)
					length = length.substr(f);
				else
					length = "";
				if (length.empty())
				{
					// 得到不文件长度, 设置为不支持多下载模式.
					m_accept_multi = false;
				}
			}

			if (!length.empty())
			{
				try
				{
					m_file_size = boost::lexical_cast<boost::int64_t>(length);
				}
				catch (boost::bad_lexical_cast &)
				{
					// 得不到正确的文件长度, 设置为不支持多下载模式.
					m_accept_multi = false;
				}
			}

			// 按文件大小分配rangefield.
			m_rangefield.reset(m_file_size);
		}

		// 是否支持长连接模式.
		std::string keep_alive;
		h.response_options().find("Connection", keep_alive);
		boost::to_lower(keep_alive);
		if (keep_alive == "keep-alive")
			m_keep_alive = true;
		else
			m_keep_alive = false;

		// 创建存储对象.
		if (!p)
			m_storage.reset(default_storage_constructor());
		else
			m_storage.reset(p());
		BOOST_ASSERT(m_storage);

		// 打开文件.
		std::string file_name = boost::filesystem::path(m_final_url.path()).leaf().string();
		m_storage->open(boost::filesystem::path(file_name), ec);
		if (ec)
		{
			return ec;
		}

		// 保存设置.
		m_settings = s;

		// 处理默认设置.
		if (m_settings.m_connections_limit == -1)
			m_settings.m_connections_limit = default_connections_limit;
		if (m_settings.m_piece_size == -1 && m_file_size != -1)
			m_settings.m_piece_size = default_piece_size;

		// 关闭stream.
		h.close(ec);
		if (ec)
		{
			return ec;
		}

		m_streams.push_back(obj);

		// 根据第1个连接返回的信息, 重新设置请求选项.
		req_opt.clear();
		if (m_keep_alive)
			req_opt.insert("Connection", "keep-alive");
		else
			req_opt.insert("Connection", "close");

		// 修改终止状态.
		m_abort = false;

		// 如果支持多点下载, 按设置创建其它http_stream.
		if (m_accept_multi)
		{
			for (int i = 1; i < m_settings.m_connections_limit; i++)
			{
				http_object_ptr p(new http_stream_object());
				http_stream_ptr ptr(new http_stream(m_io_service));
				range req_range;

				// 从文件间区中得到一段空间.
				if (!allocate_range(req_range))
				{
					// 分配空间失败, 说明可能已经没有空闲的空间提供给这个stream进行下载了
					// 直接跳过好了.
					continue;
				}

				// 保存请求区间.
				p->m_request_range = req_range;

				// 设置请求区间到请求选项中.
				req_opt.remove("Range");
				req_opt.insert("Range",
					boost::str(boost::format("bytes=%lld-%lld") % req_range.left % req_range.right));

				// 设置请求选项.
				ptr->request_options(req_opt);

				// 将连接添加到容器中.
				p->m_stream = ptr;
				m_streams.push_back(p);

				// 保存最后请求时间, 方便检查超时重置.
				p->m_last_request_time = boost::posix_time::microsec_clock::local_time();

				// 开始异步打开.
				p->m_stream->async_open(m_final_url,
					boost::bind(&multi_download::handle_open, this,
					i, p, boost::asio::placeholders::error));
			}
		}

		// 为已经第1个已经打开的http_stream发起异步数据请求, index标识为0.
		do
		{
			if (m_accept_multi)
			{
				range req_range;

				// 从文件间区中得到一段空间.
				if (!allocate_range(req_range))
				{
					// 分配空间失败, 说明可能已经没有空闲的空间提供给这个stream进行下载了
					// 直接跳过好了.
					break;
				}

				// 保存请求区间.
				obj->m_request_range = req_range;

				// 设置请求区间到请求选项中.
				req_opt.remove("Range");
				req_opt.insert("Range",
					boost::str(boost::format("bytes=%lld-%lld") % req_range.left % req_range.right));

				// 设置请求选项.
				h.request_options(req_opt);
			}
			else
			{
				range req_range;
				if (!allocate_range(req_range))
				{
					// 用于调试, 不可能执行到这里! 因为单socket模式没有其它stream和它竞争文件区间.
					BOOST_ASSERT(0);
				}

				// 保存请求范围.
				obj->m_request_range = req_range;
			}

			// 保存最后请求时间, 方便检查超时重置.
			obj->m_last_request_time = boost::posix_time::microsec_clock::local_time();

			// 开始异步打开.
			h.async_open(m_final_url,
				boost::bind(&multi_download::handle_open, this,
				0, obj, boost::asio::placeholders::error));

		} while (0);

		// 开启定时器, 执行任务.
		m_timer.async_wait(boost::bind(&multi_download::on_tick, this));

		return ec;
	}

	// TODO: 实现close.
	void close()
	{
		m_abort = true;
	}

protected:
	void handle_open(const int index,
		http_object_ptr object_ptr, const boost::system::error_code &ec)
	{
		if (ec || m_abort)
		{
			// 输出错误信息, 然后退出, 让on_tick检查到超时后重新连接.
			std::cerr << index << " handle_open: " << ec.message().c_str() << std::endl;
			return;
		}

		// 保存最后请求时间, 方便检查超时重置.
		object_ptr->m_last_request_time = boost::posix_time::microsec_clock::local_time();

		// 发起数据读取请求.
		http_stream_ptr &stream_ptr = object_ptr->m_stream;
		stream_ptr->async_read_some(boost::asio::buffer(object_ptr->m_buffer),
			boost::bind(&multi_download::handle_read, this,
			index, object_ptr,
			boost::asio::placeholders::bytes_transferred,
			boost::asio::placeholders::error));
	}

	void handle_read(const int index,
		http_object_ptr object_ptr, int bytes_transferred, const boost::system::error_code &ec)
	{
		// 保存数据.
		if (m_storage && !ec)
		{
			// 计算offset.
			boost::int64_t offset = object_ptr->m_request_range.left + object_ptr->m_bytes_transferred;

			// 使用m_storage写入.
			m_storage->write(object_ptr->m_buffer.c_array(),
				object_ptr->m_request_range.left + object_ptr->m_bytes_transferred, bytes_transferred);
		}

		// 如果发生错误或终止.
		if (ec || m_abort)
		{
			// 输出错误信息, 然后退出, 让on_tick检查到超时后重新连接.
			std::cerr << index << " handle_read: " << ec.message().c_str() << std::endl;
			return;
		}

		// 统计本次已经下载的总字节数.
		object_ptr->m_bytes_transferred += bytes_transferred;

		// 统计总下载字节数.
		object_ptr->m_bytes_downloaded += bytes_transferred;

		// 判断请求区间的数据已经下载完成, 如果下载完成, 则分配新的区间, 发起新的请求.
		if (object_ptr->m_bytes_transferred >= object_ptr->m_request_range.size())
		{
			http_stream_ptr &stream_ptr = object_ptr->m_stream;

			// 单连接模式, 表示下载完成, 终止下载.
			if (!m_accept_multi)
			{
				m_abort = true;	// 终止下载!!!
				return;
			}

			// 不支持长连接, 则创建新的连接.
			if (!m_keep_alive)
			{
				// 新建新的http_stream对象.
				stream_ptr.reset(new http_stream(m_io_service));
			}

			// 配置请求选项.
			request_opts req_opt;

			// 设置是否为长连接.
			if (m_keep_alive)
				req_opt.insert("Connection", "keep-alive");

			// 如果分配空闲空间失败, 则跳过这个socket, 并立即尝试连接这个socket.
			if (!allocate_range(object_ptr->m_request_range))
			{
				object_ptr->m_direct_reconnect = true;
				return;
			}

			// 清空计数.
			object_ptr->m_bytes_transferred = 0;

			// 插入新的区间请求.
			req_opt.insert("Range",
				boost::str(boost::format("bytes=%lld-%lld")
				% object_ptr->m_request_range.left % object_ptr->m_request_range.right));

			// 设置到请求选项中.
			stream_ptr->request_options(req_opt);

			// 保存最后请求时间, 方便检查超时重置.
			object_ptr->m_last_request_time = boost::posix_time::microsec_clock::local_time();

			// 发起异步http数据请求.
			if (!m_keep_alive)
				stream_ptr->async_open(m_final_url, boost::bind(&multi_download::handle_open, this,
					index, object_ptr, boost::asio::placeholders::error));
			else
				stream_ptr->async_request(req_opt, boost::bind(&multi_download::handle_request, this,
					index, object_ptr, boost::asio::placeholders::error));
		}
		else
		{
			// 保存最后请求时间, 方便检查超时重置.
			object_ptr->m_last_request_time = boost::posix_time::microsec_clock::local_time();

			// 继续读取数据.
			http_stream_ptr &stream_ptr = object_ptr->m_stream;
			stream_ptr->async_read_some(boost::asio::buffer(object_ptr->m_buffer),
				boost::bind(&multi_download::handle_read, this,
				index, object_ptr,
				boost::asio::placeholders::bytes_transferred,
				boost::asio::placeholders::error));
		}
	}

	void handle_request(const int index,
		http_object_ptr object_ptr, const boost::system::error_code &ec)
	{
		if (ec || m_abort)
		{
			// 输出错误信息, 然后退出, 让on_tick检查到超时后重新连接.
			std::cerr << index << " handle_request: " << ec.message().c_str() << std::endl;
			return;
		}

		// 保存最后请求时间, 方便检查超时重置.
		object_ptr->m_last_request_time = boost::posix_time::microsec_clock::local_time();

		// 发起数据读取请求.
		http_stream_ptr &stream_ptr = object_ptr->m_stream;
		stream_ptr->async_read_some(boost::asio::buffer(object_ptr->m_buffer),
			boost::bind(&multi_download::handle_read, this,
			index, object_ptr,
			boost::asio::placeholders::bytes_transferred,
			boost::asio::placeholders::error));
	}

	void on_tick()
	{
		// 每隔1秒进行一次on_tick.
		if (!m_abort)
		{
			m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(1));
			m_timer.async_wait(boost::bind(&multi_download::on_tick, this));
		}
		else
		{
			// 已经终止.
			return;
		}

		// 统计操作功能完成的http_stream的个数.
		int done = 0;

		for (std::size_t i = 0; i < m_streams.size(); i++)
		{
			http_object_ptr &object_item_ptr = m_streams[i];
			boost::posix_time::time_duration duration =
				boost::posix_time::microsec_clock::local_time() - object_item_ptr->m_last_request_time;

			if (!object_item_ptr->m_done &&	(duration > boost::posix_time::seconds(m_settings.m_time_out)
				|| object_item_ptr->m_direct_reconnect))
			{
				// 超时, 关闭并重新创建连接.
				boost::system::error_code ec;
				object_item_ptr->m_stream->close(ec);

				std::cerr << "connection: " << i << " is timeout !!!" << std::endl;

				// 重新创建http_object和http_stream.
				http_object_ptr object_ptr(new http_stream_object(*object_item_ptr));
				http_stream_ptr stream_ptr(new http_stream(m_io_service));

				// 使用新的http_stream对象.
				object_ptr->m_stream = stream_ptr;

				// 重置m_streams中的对象.
				object_item_ptr = object_ptr;

				// 配置请求选项.
				request_opts req_opt;

				// 设置是否为长连接.
				if (m_keep_alive)
					req_opt.insert("Connection", "keep-alive");

				// 继续从上次未完成的位置开始请求.
				if (m_accept_multi)
				{
					boost::int64_t begin = object_ptr->m_request_range.left + object_ptr->m_bytes_transferred;
					boost::int64_t end = object_ptr->m_request_range.left + object_ptr->m_request_range.size();

					if (end - begin == 0)
					{
						// 如果分配空闲空间失败, 则跳过这个socket.
						if (!allocate_range(object_ptr->m_request_range))
						{
							object_ptr->m_done = true;	// 已经没什么可以下载了.
							continue;
						}

						object_ptr->m_bytes_transferred = 0;
						begin = object_ptr->m_request_range.left;
						end = object_ptr->m_request_range.right;
					}

					req_opt.insert("Range",
						boost::str(boost::format("bytes=%lld-%lld") % begin % end));
				}

				// 设置到请求选项中.
				stream_ptr->request_options(req_opt);

				// 保存最后请求时间, 方便检查超时重置.
				object_item_ptr->m_last_request_time = boost::posix_time::microsec_clock::local_time();

				// 重新发起异步请求.
				stream_ptr->async_open(m_final_url, boost::bind(&multi_download::handle_open, this,
					i, object_ptr, boost::asio::placeholders::error));
			}

			// 计算已经完成操作的object.
			if (object_item_ptr->m_done)
				done++;
		}

		// 检查位图是否已经满以及异步操作是否完成.
		if (done == m_streams.size() && m_rangefield.is_full())
		{
			std::cout << "*** download completed! ***" << std::endl;
			m_abort = true;
			return;
		}
	}

	bool allocate_range(range &r)
	{
		boost::mutex::scoped_lock lock(m_rangefield_mutex);
		range temp(-1, -1);
		do
		{
			// 从文件间区中得到一段空间.
			if (!m_rangefield.out_space(temp))
				return false;

			// 用于调试.
			BOOST_ASSERT(temp != r);
			BOOST_ASSERT(temp.size() >= 0);

			// 重新计算为最大max_request_bytes大小.
			boost::int64_t max_request_bytes = m_settings.m_request_piece_num * m_settings.m_piece_size;
			if (temp.size() > max_request_bytes)
				temp.right = temp.left + max_request_bytes;

			r = temp;

			// 从m_rangefield中分配这个空间.
			if (!m_rangefield.update(temp))
				continue;
			else
				break;
		} while (!m_abort);

		// 右边边界减1, 因为http请求的区间是包含右边界值, 下载时会将right下标位置的字节下载.
		if (--r.right < r.left)
			return false;

		return true;
	}

private:
	// io_service引用.
	boost::asio::io_service &m_io_service;

	// 每一个http_stream_obj是一个http连接.
	std::vector<http_object_ptr> m_streams;

	// 最终的url, 如果有跳转的话, 是跳转最后的那个url.
	url m_final_url;

	// 是否支持多点下载.
	bool m_accept_multi;

	// 是否支持长连接.
	bool m_keep_alive;

	// 文件大小, 如果没有文件大小值为-1.
	boost::int64_t m_file_size;

	// 当前用户设置.
	settings m_settings;

	// 定时器, 用于定时执行一些任务, 比如检查连接是否超时之类.
	boost::asio::deadline_timer m_timer;

	// 下载数据存储接口指针, 可由用户定义, 并在open时指定.
	boost::scoped_ptr<storage_interface> m_storage;

	// 文件区间图, 每次请求将由m_rangefield来分配空间区间.
	rangefield m_rangefield;

	// 保证分配空闲区间的唯一性.
	boost::mutex m_rangefield_mutex;

	// 是否中止工作.
	bool m_abort;
};

} // avhttp

#endif // MULTI_DOWNLOAD_HPP__
