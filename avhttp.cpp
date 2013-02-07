#include <iostream>

// 异步, 
// 同步,
// 
// 
// 
// 
// 

#include "url.hpp"
#include "avhttp.hpp"
#include "detail/error_codec.hpp"


int main(int argc, char* argv[])
{
	boost::asio::io_service io;
	avhttp::http_stream h(io);
	h.open("http://weibo.com//?topnav=1&wvr=5&mod=logo");
	io.run();

	return 0;
}

