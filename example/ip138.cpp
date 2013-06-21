#include <iostream>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include "avhttp.hpp"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "usage: " << argv[0] << " <domain/ip>\n";
		return -1;
	}

	boost::asio::io_service io;
	boost::system::error_code ec;

	std::string ip = argv[1];
	std::string query_url = "http://ip138.com/ips138.asp?ip=" + ip;

	avhttp::http_stream h(io);
	h.open(query_url, ec);
	if (ec)
		return -1;

	std::string result;
	boost::asio::streambuf response;
	while (!ec)
	{
		// 每次读取一行.
		std::size_t bytes_transferred = boost::asio::read_until(h, response, "\r\n", ec);
		if (bytes_transferred > 0)
		{
			std::string s;
			// 查找是否有ul1的标签.
			s.resize(response.size());
			response.sgetn(&s[0], bytes_transferred);
			std::size_t pos = s.find("<ul class=\"ul1\"><li>");
			if (pos == std::string::npos)
				continue;
			// 匹配出地址信息.
			boost::cmatch what;
			boost::regex ex("<ul class=\"ul1\"><li>(.*)?<\\/li><li>");
			if(boost::regex_search(s.c_str(), what, ex))
			{
				result = std::string(what[1]);
				std::string gbk_ex;
				gbk_ex.push_back('\xA3');
				gbk_ex.push_back('\xBA');
				gbk_ex += "(.*)";
				ex.assign(gbk_ex);
				if(boost::regex_search(result.c_str(), what, ex))
					result = std::string(what[1]);
				break;
			}
		}
	}

	// 输出地址信息.
	if (!result.empty())
		std::cout << result << std::endl;

	return 0;
}
