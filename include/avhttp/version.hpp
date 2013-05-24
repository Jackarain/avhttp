//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __VERSION_HPP__
#define __VERSION_HPP__

#define AVHTTP_VERSION_MAJOR 2
#define AVHTTP_VERSION_MINOR 8
#define AVHTTP_VERSION_TINY 0

// the format of this version is: MMmmtt
// M = Major version, m = minor version, t = tiny version
#define AVHTTP_VERSION_NUM ((AVHTTP_VERSION_MAJOR * 10000) + (AVHTTP_VERSION_MINOR * 100) + AVHTTP_VERSION_TINY)
#define AVHTTP_VERSION "2.8.0"
#define AVHTTP_VERSION_MIME "avhttp/"##AVHTTP_VERSION
// #define AVHTTP_REVISION "$Git-Rev$"


#endif // __VERSION_HPP__
