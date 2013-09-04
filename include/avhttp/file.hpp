//
// file.hpp
// ~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FILE_HPP__
#define __FILE_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/noncopyable.hpp>
#include <boost/filesystem/fstream.hpp>

#include "avhttp/storage_interface.hpp"

namespace avhttp {

using std::ios;

class file
	: public storage_interface
	, public boost::noncopyable
{
public:
	file() {}
	virtual ~file() { close(); }

public:

	// 打开指定的文件, 如果不存在则创建.
	// 失败将抛出一个boost::system::system_error异常.
	virtual void open(const fs::path& file_path)
	{
		boost::system::error_code ec;
		open(file_path, ec);
		if (ec)
		{
			boost::throw_exception(boost::system::system_error(ec));
		}
	}

	// 打开指定的文件, 如果不存在则创建.
	// @param file_path指定了文件名路径信息.
	// @param ec在出错时保存了详细的错误信息.
	virtual void open(const fs::path& file_path, boost::system::error_code& ec)
	{
		ec = boost::system::error_code();
		m_fstream.open(file_path, ios::binary|ios::in|ios::out);
		if (!m_fstream.is_open())
		{
			m_fstream.clear();
			m_fstream.open(file_path, ios::trunc|ios::binary|ios::out|ios::in);
		}
		if (!m_fstream.is_open())
		{
			ec = boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor);
			return;
		}
		m_fstream.clear();
	}

	///是否打开.
	inline bool is_open() const
	{
		return m_fstream.is_open();
	}

	// 关闭存储组件.
	virtual void close()
	{
		m_fstream.close();
	}

	// 写入数据到文件.
	// @param buf是需要写入的数据缓冲.
	// @param size指定了写入的数据缓冲大小.
	// @返回值为实际写入的字节数, 返回-1表示写入失败.
	// 备注: 在文件指针当前位置写入, 写入完成将自动移动文件指针到完成位置, 保证和fwrite行为一至.
	virtual std::streamsize write(const char* buf, int size)
	{
		m_fstream.write(buf, size);
		if (m_fstream.good())
		{
			m_fstream.flush();
			return size;
		}
		return 0;
	}

	// 写入数据到文件.
	// @param buf是需要写入的数据缓冲.
	// @param offset是写入的偏移位置.
	// @param size指定了写入的数据缓冲大小.
	// @返回值为实际写入的字节数, 返回-1表示写入失败.
	virtual std::streamsize write(const char* buf, boost::uint64_t offset, int size)
	{
		m_fstream.seekp(offset, ios::beg);
		m_fstream.write(buf, size);
		if (m_fstream.good())
		{
			m_fstream.flush();
			return size;
		}
		return 0;
	}

	// 从文件读取数据.
	// @param buf是需要读取的数据缓冲.
	// @param size指定了读取的数据缓冲大小.
	// @返回值为实际读取的字节数, 返回-1表示读取失败.
	// 备注: 在文件指针当前位置开始读取, 读取完成将自动移动文件指针到完成位置, 保证和fread行为一至.
	virtual std::streamsize read(char* buf, int size)
	{
		m_fstream.read(buf, size);
		if (m_fstream.good())
		{
			return m_fstream.gcount();
		}
		return 0;
	}

	// 从文件读取数据.
	// @param buf是需要读取的数据缓冲.
	// @param offset是读取的偏移位置.
	// @param size指定了读取的数据缓冲大小.
	// @返回值为实际读取的字节数, 返回-1表示读取失败.
	virtual std::streamsize read(char* buf, boost::uint64_t offset, int size)
	{
		m_fstream.seekg(offset, ios::beg);
		m_fstream.read(buf, size);
		if (m_fstream.good())
		{
			return m_fstream.gcount();
		}
		return 0;
	}

	virtual void getline(std::string& str)
	{
		std::getline(m_fstream, str);
	}

	virtual bool eof() const
	{
		return m_fstream.eof();
	}

protected:
	boost::filesystem::fstream m_fstream;
};

// 默认存储对象.
static storage_interface* default_storage_constructor()
{
	return new file();
}

}

#endif // __FILE_HPP__
