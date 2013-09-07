#include <iostream>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "avhttp.hpp"

class image_bin : public boost::noncopyable
{
public:
	image_bin(boost::asio::io_service& io, const std::string& filename, const std::string& nickname)
		: m_io(io)
		, m_file_upload(io)
		, m_filename(filename)
	{
		avhttp::file_upload::form_args agrs;
		boost::system::error_code ec;
		avhttp::request_opts opts;

		opts.insert("Referer", "http://imagebin.org/index.php?page=add");
		opts.insert(avhttp::http_options::user_agent, "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.72 Safari/537.36");
		opts.insert("Origin", "http://imagebin.org");
		opts.insert("Cache-Control", "max-age=0");
		opts.insert("Accept-Language", "zh-CN,zh;q=0.8");

		m_file_upload.request_option(opts);

		agrs["nickname"] = nickname;
		agrs["remember_nickname"] = "Y";
		agrs["title"] = boost::filesystem::path(m_filename).leaf().string();
		agrs["description"] = "Upload by avhttp";
		agrs["disclaimer_agree"] = "Y";
		agrs["Submit"] = "Submit";
		agrs["mode"] = "add";

		m_file_upload.async_open("http://imagebin.org/index.php",
			m_filename, "image", agrs,
			boost::bind(&image_bin::open_handle, this, boost::asio::placeholders::error));
	}

	void open_handle(boost::system::error_code ec)
	{
		if (!ec)
		{
			m_file.open(m_filename, ec);
			std::streamsize readed = m_file.read(m_buffer.data(), 1024);
			boost::asio::async_write(m_file_upload, boost::asio::buffer(m_buffer, readed),
				boost::bind(&image_bin::write_handle, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}

	void write_handle(boost::system::error_code ec, std::size_t bytes_transfered)
	{
		if (!ec)
		{
			if (m_file.eof())
			{
				m_file_upload.async_write_tail(boost::bind(
					&image_bin::tail_handle, this, boost::asio::placeholders::error));
			}
			else
			{
				std::streamsize readed = m_file.read(m_buffer.data(), 1024);
				boost::asio::async_write(m_file_upload, boost::asio::buffer(m_buffer, readed),
					boost::bind(&image_bin::write_handle, this,
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
		}
	}

	void tail_handle(boost::system::error_code ec)
	{
		if (!ec)
		{
			avhttp::http_stream& http = m_file_upload.get_http_stream();
			std::cout << &http;
		}
	}

private:
	boost::asio::io_service& m_io;
	avhttp::file_upload m_file_upload;
	avhttp::default_storge m_file;
	boost::array<char, 1024> m_buffer;
	std::string m_filename;
};

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "usage: " << argv[0] << " <filename> [nickname]\n";
		return -1;
	}

	std::string extension = boost::filesystem::path(argv[1]).extension().string();
	boost::to_lower(extension);
	if (extension != ".png" &&
		extension != ".jpg" &&
		extension != ".jpeg" &&
		extension != ".gif" &&
		extension != ".jpe")
	{
		std::cerr << "You must provide a image!\n";
		return -1;
	}

	std::string nickname = "Jackarain";
	if (argc >= 3)
	{
		nickname = argv[2];
	}

	boost::asio::io_service io;
	image_bin img(io, argv[1], nickname);
	io.run();

	return 0;
}
