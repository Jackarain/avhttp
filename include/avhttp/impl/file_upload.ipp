//
// file_upload.ipp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef AVHTTP_FILE_UPLOAD_IPP
#define AVHTTP_FILE_UPLOAD_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/yield.hpp>
#include "avhttp/http_stream.hpp"

namespace avhttp {

#define FORMBOUNDARY "----AvHttpFormBoundaryamFja2FyYWlu"

std::size_t calc_content_length(const std::string& filename, const std::string& file_of_form,
	const file_upload::form_args& args, boost::system::error_code& ec)
{
	std::string boundary = FORMBOUNDARY;
	std::string short_filename = fs::path(filename).leaf().string();
	std::size_t content_length = fs::file_size(filename, ec);
	std::size_t boundary_size = boundary.size() + 4; // 4 是指 "--" + "\r\n"

	// 各属性选项长度.
	file_upload::form_args::const_iterator i = args.begin();
	for (; i != args.end(); i++)
	{
		content_length += boundary_size;
		content_length += std::string("Content-Disposition: form-data; name=\"\"\r\n\r\n\r\n").size();
		content_length += i->first.size();
		content_length += i->second.size();
	}
	// 文件名选项长度.
	content_length += boundary_size;
	content_length += std::string("Content-Disposition: form-data; name=\"\"; filename=\"\"\r\n"
		"Content-Type: application/x-msdownload\r\n\r\n").size();
	content_length += file_of_form.size();
	content_length += short_filename.size();
	content_length += 4;
	// 结束边界长度.
	content_length += boundary_size;

	return content_length;
}

file_upload::file_upload(boost::asio::io_service& io)
	: m_io_service(io)
	, m_http_stream(io)
{}

file_upload::~file_upload()
{}

template <typename Handler>
class file_upload::open_coro : boost::asio::coroutine
{
public:
	open_coro(http_stream& http, const std::string& url, const std::string& filename,
		const std::string& file_of_form, const form_args& args, std::string& boundary, Handler handler)
		: m_handler(handler)
		, m_http_stream(http)
		, m_filename(filename)
		, m_file_of_form(file_of_form)
		, m_form_args(args)
		, m_boundary(boundary)
	{
		 boost::system::error_code ec;
		request_opts opts;

		// 设置为POST模式.
		opts.insert(http_options::request_method, "POST");
		opts.insert("Expect", "100-continue");

		// 计算Content-Length.
		std::size_t content_length = calc_content_length(filename, file_of_form, args, ec);
		if (ec)
		{
			m_handler(ec);
			return;
		}

		m_content_disposition = boost::make_shared<std::string>();

		// 转换成字符串.
		std::ostringstream content_length_stream;
		content_length_stream.imbue(std::locale("C"));
		content_length_stream << content_length;
		opts.insert(http_options::content_length, content_length_stream.str());

		// 添加边界等选项并打开url.
		m_boundary = FORMBOUNDARY;
		opts.insert(http_options::content_type, "multipart/form-data; boundary=" + m_boundary);
		m_boundary = "--" + m_boundary + "\r\n";	// 之后都是单行的分隔.
		m_http_stream.request_options(opts);
		m_http_stream.async_open(url, *this);
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transfered = 0)
	{
		// 出错, 如果是errc::continue_request则忽略.
		if (ec && ec != errc::continue_request)
		{
			m_handler(ec);
			return;
		}

		reenter (this)
		{
			// 循环发送表单参数.
			m_iter = m_form_args.begin();
			for (; m_iter != m_form_args.end(); m_iter++)
			{
				yield boost::asio::async_write(m_http_stream, boost::asio::buffer(m_boundary), *this);
				// 发送 Content-Disposition.
				*m_content_disposition = "Content-Disposition: form-data; name=\""
					+ m_iter->first + "\"\r\n\r\n";
				*m_content_disposition += m_iter->second;
				*m_content_disposition += "\r\n";
				yield boost::asio::async_write(m_http_stream, boost::asio::buffer(*m_content_disposition),
					*this);
			}

			// 发送文件名.
			yield boost::asio::async_write(m_http_stream, boost::asio::buffer(m_boundary), *this);
			*m_content_disposition = "Content-Disposition: form-data; name=\""
				+ m_file_of_form + "\"" + "; filename=" + "\"" + m_filename + "\"\r\n"
				+ "Content-Type: application/x-msdownload\r\n\r\n";
			yield boost::asio::async_write(m_http_stream, boost::asio::buffer(*m_content_disposition), *this);
			// 回调用户handler.
			m_handler(ec);
		}
	}

private:
	Handler m_handler;
	http_stream& m_http_stream;
	std::string m_filename;
	const form_args& m_form_args;
	std::string m_file_of_form;
	std::string& m_boundary;
	boost::shared_ptr<std::string> m_content_disposition;
	form_args::const_iterator m_iter;
};

template <typename Handler>
file_upload::open_coro<Handler>
file_upload::make_open_coro(const std::string& url, const std::string& filename,
	const std::string& file_of_form, const form_args& args, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	m_form_args = args;
	return open_coro<Handler>(m_http_stream,
		url, filename, file_of_form, m_form_args, m_boundary, handler);
}

template <typename Handler>
void file_upload::async_open(const std::string& url, const std::string& filename,
	const std::string& file_of_form, const form_args& args, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	make_open_coro<Handler>(url, filename, file_of_form, args, handler);
}

void file_upload::open(const std::string& url, const std::string& filename,
	const std::string& file_of_form, const form_args& args, boost::system::error_code& ec)
{
	request_opts& opts = m_request_opts;
	m_form_args = args;

	// 设置边界字符串.
	m_boundary = FORMBOUNDARY;

	// 作为发送名字.
	std::string short_filename = fs::path(filename).leaf().string();

	// 设置为POST模式.
	opts.insert(http_options::request_method, "POST");

	// 计算Content-Length.
	std::size_t content_length = calc_content_length(filename, file_of_form, args, ec);
	if (ec)
	{
		return;
	}

	// 转换成字符串.
	std::ostringstream content_length_stream;
	content_length_stream.imbue(std::locale("C"));
	content_length_stream << content_length;
	opts.insert(http_options::content_length, content_length_stream.str());

	opts.insert("Expect", "100-continue");

	// 添加边界等选项并打开url.
	opts.insert(http_options::content_type, "multipart/form-data; boundary=" + m_boundary);
	m_boundary = "--" + m_boundary + "\r\n";	// 之后都是单行的分隔.
	m_http_stream.request_options(opts);
	m_http_stream.open(url, ec);

	// 出错, 如果是errc::continue_request则忽略.
	if (ec && ec != errc::continue_request)
	{
		return;
	}

	// 循环发送表单参数.
	std::string content_disposition;
	form_args::const_iterator i = args.begin();
	for (; i != args.end(); i++)
	{
		boost::asio::write(m_http_stream, boost::asio::buffer(m_boundary), ec);
		if (ec)
		{
			return;
		}
		// 发送 Content-Disposition.
		content_disposition = "Content-Disposition: form-data; name=\""
			+ i->first + "\"\r\n\r\n";
		content_disposition += i->second;
		content_disposition += "\r\n";
		boost::asio::write(m_http_stream, boost::asio::buffer(content_disposition), ec);
		if (ec)
		{
			return;
		}
	}

	// 发送文件名.
	boost::asio::write(m_http_stream, boost::asio::buffer(m_boundary), ec);
	if (ec)
	{
		return;
	}
	content_disposition = "Content-Disposition: form-data; name=\""
		+ file_of_form + "\"" + "; filename=" + "\"" + short_filename + "\"\r\n"
		+ "Content-Type: application/x-msdownload\r\n\r\n";
	boost::asio::write(m_http_stream, boost::asio::buffer(content_disposition), ec);
	if (ec)
	{
		return;
	}
}

void file_upload::open(const std::string& url, const std::string& filename,
	const std::string& file_of_form, const form_args& args)
{
	boost::system::error_code ec;
	open(url, filename, file_of_form, args, ec);
	if (ec)
	{
		boost::throw_exception(boost::system::system_error(ec));
	}
}

template <typename ConstBufferSequence>
std::size_t file_upload::write_some(const ConstBufferSequence& buffers)
{
	return m_http_stream.write_some(buffers);
}

template <typename ConstBufferSequence>
std::size_t file_upload::write_some(const ConstBufferSequence& buffers,
	boost::system::error_code& ec)
{
	return m_http_stream.write_some(buffers, ec);
}

void file_upload::write_tail(boost::system::error_code& ec)
{
	// 发送结尾.
	m_boundary = "\r\n--" FORMBOUNDARY "--\r\n";
	boost::asio::write(m_http_stream, boost::asio::buffer(m_boundary), ec);
	// 继续读取http header.
	m_http_stream.receive_header(ec);
}

void file_upload::write_tail()
{
	// 发送结尾.
	m_boundary = "\r\n--" FORMBOUNDARY "--\r\n";
	boost::asio::write(m_http_stream, boost::asio::buffer(m_boundary));
	// 继续读取http header.
	m_http_stream.receive_header();
}

void file_upload::request_option(request_opts& opts)
{
	m_request_opts = opts;
}

http_stream& file_upload::get_http_stream()
{
	return m_http_stream;
}

} // namespace avhttp

#include <boost/asio/unyield.hpp>

#endif // AVHTTP_FILE_UPLOAD_IPP
