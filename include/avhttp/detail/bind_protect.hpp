//
// bind_protect.hpp
// ~~~~~~~~~~~~~~~~
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2009 Steven Watanabe
//  Copyright (c) 2011 Jack (jack.wgm at gmail.com)
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __BIND_PROTECT_H__
#define __BIND_PROTECT_H__

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

namespace avhttp {
namespace detail {

template<class F> class bind_protected_t
{
public:
	typedef typename F::result_type result_type;
	explicit bind_protected_t(F f): f_(f)
	{}

	result_type operator()()
	{
		return f_();
	}

	result_type operator()() const
	{
		return f_();
	}

	template<class A1> result_type operator()(A1 & a1)
	{
		return f_(a1);
	}

	template<class A1> result_type operator()(A1 & a1) const
	{
		return f_(a1);
	}


#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1> result_type operator()(const A1 & a1)
	{
		return f_(a1);
	}

	template<class A1> result_type operator()(const A1 & a1) const
	{
		return f_(a1);
	}

#endif

	template<class A1, class A2> result_type operator()(A1 & a1, A2 & a2)
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 & a1, A2 & a2) const
	{
		return f_(a1, a2);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2> result_type operator()(A1 const & a1, A2 & a2)
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 const & a1, A2 & a2) const
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 & a1, A2 const & a2)
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 & a1, A2 const & a2) const
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 const & a1, A2 const & a2)
	{
		return f_(a1, a2);
	}

	template<class A1, class A2> result_type operator()(A1 const & a1, A2 const & a2) const
	{
		return f_(a1, a2);
	}

#endif

	template<class A1, class A2, class A3> result_type operator()(A1 & a1, A2 & a2, A3 & a3)
	{
		return f_(a1, a2, a3);
	}

	template<class A1, class A2, class A3> result_type operator()(A1 & a1, A2 & a2, A3 & a3) const
	{
		return f_(a1, a2, a3);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3)
	{
		return f_(a1, a2, a3);
	}

	template<class A1, class A2, class A3> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3) const
	{
		return f_(a1, a2, a3);
	}

#endif

	template<class A1, class A2, class A3, class A4> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4)
	{
		return f_(a1, a2, a3, a4);
	}

	template<class A1, class A2, class A3, class A4> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4) const
	{
		return f_(a1, a2, a3, a4);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4)
	{
		return f_(a1, a2, a3, a4);
	}

	template<class A1, class A2, class A3, class A4> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4) const
	{
		return f_(a1, a2, a3, a4);
	}

#endif

	template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5)
	{
		return f_(a1, a2, a3, a4, a5);
	}

	template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5) const
	{
		return f_(a1, a2, a3, a4, a5);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5)
	{
		return f_(a1, a2, a3, a4, a5);
	}

	template<class A1, class A2, class A3, class A4, class A5> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5) const
	{
		return f_(a1, a2, a3, a4, a5);
	}

#endif

	template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6)
	{
		return f_(a1, a2, a3, a4, a5, a6);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6) const
	{
		return f_(a1, a2, a3, a4, a5, a6);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6)
	{
		return f_(a1, a2, a3, a4, a5, a6);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6) const
	{
		return f_(a1, a2, a3, a4, a5, a6);
	}

#endif

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7);
	}

#endif

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8);
	}

#endif

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) && !BOOST_WORKAROUND(__EDG_VERSION__, <= 238)

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9)
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}

	template<class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(A1 const & a1, A2 const & a2, A3 const & a3, A4 const & a4, A5 const & a5, A6 const & a6, A7 const & a7, A8 const & a8, A9 const & a9) const
	{
		return f_(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}

#endif

private:
	F f_;
};

} // namespace detail.
} // namespace avhttp.

#endif // __BIND_PROTECT_H__
