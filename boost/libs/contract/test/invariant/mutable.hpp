
// no #include guard

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

// Test error if non-static inv declared mutable (unless PERMISSIVE #defined).

#include <boost/contract/public_function.hpp>
#include <boost/contract/check.hpp>

struct a {
    void invariant() {}

    void f() {
        // Same for ctor and dtor (because they all use check_pre_post_inv).
        boost::contract::check c = boost::contract::public_function(this);
    }
};

int main() {
    a aa;
    aa.f();
    return 0;
}
