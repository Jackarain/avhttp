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

#include "storage_interface.hpp"

namespace avhttp
{

class file
	: public storage_interface
	, public boost::noncopyable
{
public:
	file() {}
	virtual ~file() { close(); }

public:

	// 存储组件初始化.
	// @param file_path指定了文件名路径信息.
	// @param ec在出错时保存了详细的错误信息.
	virtual void open(fs::path &file_path, boost::system::error_code &ec)
	{

	}

	// 关闭存储组件.
	virtual void close()
	{

	}

	// 写入数据.
	// @param buf是需要写入的数据缓冲.
	// @param offset是写入的偏移位置.
	// @param size指定了写入的数据缓冲大小.
	// @返回值为实际写入的字节数, 返回-1表示写入失败.
	virtual int write(const char *buf, boost::uint64_t offset, int size)
	{
		return -1;
	}

	// 读取数据.
	// @param buf是需要读取的数据缓冲.
	// @param offset是读取的偏移位置.
	// @param size指定了读取的数据缓冲大小.
	// @返回值为实际读取的字节数, 返回-1表示读取失败.
	virtual int read(char *buf, boost::uint64_t offset, int size)
	{
		return -1;
	}

protected:

};

// 默认存储对象.
static storage_interface* default_storage_constructor()
{
	return new file();
}

}

#endif // __FILE_HPP__
