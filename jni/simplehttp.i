%module simplehttp
%{
#include <../avhttp.hpp>
%}

%include <std_string.i>

namespace avhttp {
  std::string fetch(const std::string& url, const std::string& proxy);
}

