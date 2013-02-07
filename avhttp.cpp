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
	h.lowest_layer("https");
	return 0;
}

