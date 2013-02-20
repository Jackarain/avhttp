#include <iostream>
#include <boost/array.hpp>
#include "avhttp.hpp"

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

		d.open(argv[1]);

		io.run();
	}
	catch (boost::system::system_error &e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
