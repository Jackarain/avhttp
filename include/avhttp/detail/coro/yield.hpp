//
// yield.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coro.hpp"

#ifndef reenter
# define reenter(c) CORO_REENTER(c)
#endif

#ifndef _yield
# define _yield CORO_YIELD
#endif

#ifndef _fork
# define _fork CORO_FORK
#endif
