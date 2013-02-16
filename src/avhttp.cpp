#include <iostream>
#include <boost/array.hpp>
#include "avhttp.hpp"

class downloader
{
public:
	downloader(boost::asio::io_service &io)
		: m_io_service(io)
		, m_stream(io)
	{
		avhttp::request_opts opt;
		opt.insert("Range", "bytes=0-2");
		m_stream.request_options(opt);
		m_stream.async_open("http://www.boost.org/LICENSE_1_0.txt",
			boost::bind(&downloader::handle_open, this, boost::asio::placeholders::error));
	}
	~downloader()
	{}

public:
	void handle_open(const boost::system::error_code &ec)
	{
		if (!ec)
		{
			m_stream.async_read_some(boost::asio::buffer(m_buffer),
				boost::bind(&downloader::handle_read, this,
				boost::asio::placeholders::bytes_transferred,
				boost::asio::placeholders::error));
		}
	}

	void handle_read(int bytes_transferred, const boost::system::error_code &ec)
	{
		if (!ec)
		{
			std::cout.write(m_buffer.data(), bytes_transferred);
			m_stream.async_read_some(boost::asio::buffer(m_buffer),
				boost::bind(&downloader::handle_read, this,
				boost::asio::placeholders::bytes_transferred,
				boost::asio::placeholders::error));
		}
	}

private:
	boost::asio::io_service &m_io_service;
	avhttp::http_stream m_stream;
	boost::array<char, 1024> m_buffer;
};

int main(int argc, char* argv[])
{
	boost::asio::io_service io;

	downloader d(io);

// 	avhttp::http_stream h(io);
// 	avhttp::request_opts opt;
// 
// 	opt.insert("Connection", "close");
// 	h.request_options(opt);
// 
// 	// h.open("http://w.qq.com/cgi-bin/get_group_pic?pic={64A234EE-8657-DA63-B7F4-C7718460F461}.gif");

// 	h.async_open("http://w.qq.com/cgi-bin/get_group_pic?pic={64A234EE-8657-DA63-B7F4-C7718460F461}.gif"/*"http://www.boost.org/LICENSE_1_0.txt"*/,
// 		boost::bind(&handle_open, boost::ref(h), boost::asio::placeholders::error));

	io.run();

	return 0;
}

