//
// upload.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003, Arvid Norberg All rights reserved.
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef AVHTTP_UPLOAD_HPP
#define AVHTTP_UPLOAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/noncopyable.hpp>
#include "avhttp/http_stream.hpp"

namespace avhttp {

// WebForm文件上传组件.
// 根据RFC1867(http://www.servlets.com/rfcs/rfc1867.txt)实现.
class file_upload : public boost::noncopyable
{
public:
	typedef std::map<std::string, std::string> form_agrs;

	/// Constructor.
	AVHTTP_DECL explicit file_upload(boost::asio::io_service& io)
		: m_io_service(io)
		, m_http_stream(io)
	{}

	/// Destructor.
	AVHTTP_DECL virtual ~file_upload()
	{}

	///打开文件上传.
	// @param url指定上传文件的url.
	// @param filename指定的上传文件名.
	// @param file_of_form在web form中的上传文件名字段.
	// @param args其它各上传文件字段.
	// @param ec错误信息.
	// @begin example
	//  avhttp::file_upload f(io_service);
	//  file_upload::form_agrs fields;
	//  fields["username"] = "Cai";
	//  boost::system::error_code ec;
	//  f.open("http://example.upload/upload.cgi", "cppStudy.tar.bz2",
	//    "file", fields, ec);
	// @end example
	AVHTTP_DECL void open(const std::string& url, const std::string& filename,
		const std::string& file_of_form, const form_agrs& agrs, boost::system::error_code& ec)
	{
		request_opts opts;

		// 添加边界等选项并打开url.
		m_boundary = "----AvHttpFormBoundaryamFja2FyYWlu";
		opts.insert(http_options::content_type, "multipart/form-data; boundary=" + m_boundary);
		m_boundary = "--" + m_boundary + "\r\n";	// 之后都是单行的分隔.
		m_http_stream.request_options(opts);
		m_http_stream.open(url, ec);
		if (ec)
		{
			return;
		}

		// 循环发送表单参数.
		std::string content_disposition;
		form_agrs::const_iterator i = agrs.begin();
		for (; i != agrs.end(); i++)
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
			+ file_of_form + "\"" + "; filename=" + "\"" + filename + "\"\r\n"
			+ "Content-Type: application/x-msdownload\r\n\r\n";
		boost::asio::write(m_http_stream, boost::asio::buffer(content_disposition), ec);
		if (ec)
		{
			return;
		}
	}

	///打开文件上传.
	// @param url指定上传文件的url.
	// @param filename指定的上传文件名.
	// @param file_of_form在web form中的上传文件名字段.
	// @param args其它各上传文件字段.
	// 失败将抛出一个boost::system::system_error异常.
	// @begin example
	//  avhttp::file_upload f(io_service);
	//  file_upload::form_agrs fields;
	//  fields["username"] = "Cai";
	//  boost::system::error_code ec;
	//  f.open("http://example.upload/upload.cgi", "cppStudy.tar.bz2",
	//    "file", fields, ec);
	// @end example
	AVHTTP_DECL void open(const std::string& url, const std::string& filename,
		const std::string& file_of_form, const form_agrs& agrs)
	{
		boost::system::error_code ec;
		open(url, filename, file_of_form, agrs, ec);
		if (ec)
		{
			boost::throw_exception(boost::system::system_error(ec));
		}
	}

	///发送一些上传的文件数据.
	// @param buffers是一个或多个用于发送数据缓冲. 这个类型必须满足ConstBufferSequence, 参考文档:
	// http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/reference/ConstBufferSequence.html
	// @返回实现发送的数据大小.
	// @备注: 该函数将会阻塞到一直等待数据被发送或发生错误时才返回.
	// write_some不保证发送完所有数据, 用户需要根据返回值来确定已经发送的数据大小.
	// @begin example
	//  try
	//  {
	//    std::size bytes_transferred = s.write_some(boost::asio::buffer(data, size));
	//  }
	//  catch (boost::asio::system_error& e)
	//  {
	//    std::cerr << e.what() << std::endl;
	//  }
	//  ...
	// @end example
	// 关于示例中的boost::asio::buffer用法可以参考boost中的文档. 它可以接受一个
	// boost.array或std.vector作为数据容器.
	template <typename ConstBufferSequence>
	std::size_t write_some(const ConstBufferSequence& buffers)
	{
		return m_http_stream.write_some(buffers);
	}

	///发送一些上传的文件数据.
	// @param buffers是一个或多个用于发送数据缓冲. 这个类型必须满足ConstBufferSequence, 参考文档:
	// http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/reference/ConstBufferSequence.html
	// @返回实现发送的数据大小.
	// @备注: 该函数将会阻塞到一直等待数据被发送或发生错误时才返回.
	// write_some不保证发送完所有数据, 用户需要根据返回值来确定已经发送的数据大小.
	// @begin example
	//  boost::system::error_code ec;
	//  std::size bytes_transferred = s.write_some(boost::asio::buffer(data, size), ec);
	//  ...
	// @end example
	// 关于示例中的boost::asio::buffer用法可以参考boost中的文档. 它可以接受一个
	// boost.array或std.vector作为数据容器.
	template <typename ConstBufferSequence>
	std::size_t write_some(const ConstBufferSequence& buffers,
		boost::system::error_code& ec)
	{
		return m_http_stream.write_some(buffers, ec);
	}

	///发送结尾行.
	// 失败将抛出一个boost::system::system_error异常.
	void write_Tail()
	{
		boost::asio::write(m_http_stream, boost::asio::buffer(m_boundary));
	}

private:

	// io_service引用.
	boost::asio::io_service& m_io_service;

	// http_stream对象.
	http_stream m_http_stream;

	// 边界符.
	std::string m_boundary;
};

} // namespace avhttp

#endif // AVHTTP_UPLOAD_HPP
