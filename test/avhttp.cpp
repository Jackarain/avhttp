#include <iostream>
#include <boost/array.hpp>
#include <boost/progress.hpp>
#include <cmath>

#include "avhttp.hpp"

std::string to_string(int v, int width)
{
	std::stringstream s;
	s.flags(std::ios_base::right);
	s.width(width);
	s.fill(' ');
	s << v;
	return s.str();
}

std::string& to_string(float v, int width, int precision = 3)
{
	// this is a silly optimization
	// to avoid copying of strings
	enum { num_strings = 20 };
	static std::string buf[num_strings];
	static int round_robin = 0;
	std::string& ret = buf[round_robin];
	++round_robin;
	if (round_robin >= num_strings) round_robin = 0;
	ret.resize(20);
	int size = sprintf(&ret[0], "%*.*f", width, precision, v);
	ret.resize((std::min)(size, width));
	return ret;
}

std::string add_suffix(float val, char const* suffix = 0)
{
	std::string ret;
	if (val == 0)
	{
		ret.resize(4 + 2, ' ');
		if (suffix) ret.resize(4 + 2 + strlen(suffix), ' ');
		return ret;
	}

	const char* prefix[] = {"kB", "MB", "GB", "TB"};
	const int num_prefix = sizeof(prefix) / sizeof(const char*);
	for (int i = 0; i < num_prefix; ++i)
	{
		val /= 1024.f;
		if (std::fabs(val) < 1024.f)
		{
			ret = to_string(val, 4);
			ret += prefix[i];
			if (suffix) ret += suffix;
			return ret;
		}
	}
	ret = to_string(val, 4);
	ret += "PB";
	if (suffix) ret += suffix;
	return ret;
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "usage: " << argv[0] << " <url>\n";
		return -1;
	}

	try {
		boost::asio::io_service io;
		avhttp::multi_download d(io);

		d.start(argv[1]);
		if (d.file_size() != -1)
			std::cout << "file \'" << d.file_name().c_str() <<
			"\' size is: " << "(" << d.file_size() << " bytes) " << add_suffix(d.file_size()).c_str() << std::endl;

		boost::thread t(boost::bind(&boost::asio::io_service::run, &io));

		if (d.file_size() != -1)
		{
			boost::progress_display show_progress(d.file_size());
			while (show_progress.count() != d.file_size())
			{
				boost::this_thread::sleep(boost::posix_time::millisec(200));
				show_progress += (d.bytes_download() - show_progress.count());
			}
		}

		t.join();

		std::cout << "\n*** download completed! ***\n";
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
