#ifndef __SOCKET_TYPE_H__
#define __SOCKET_TYPE_H__

#pragma once

#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/remove.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/size.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

#include <boost/type_traits/add_pointer.hpp>
#include <boost/noncopyable.hpp>

namespace avhttp {
namespace detail {

#define VARIANT_LIMIT 2

struct delete_visitor
	: boost::static_visitor<>
{
	template <class T>
	void operator()(T* p) const
	{
		delete p;
	}

	void operator()(boost::blank) const
	{}
};

template <
	BOOST_PP_ENUM_BINARY_PARAMS(
	VARIANT_LIMIT, class S, = boost::mpl::void_ BOOST_PP_INTERCEPT
	)
>
class variant_stream : boost::noncopyable
{
public:
	typedef BOOST_PP_CAT(boost::mpl::vector, VARIANT_LIMIT)<
		BOOST_PP_ENUM_PARAMS(VARIANT_LIMIT, S)
	> types0;

	typedef typename boost::mpl::remove<types0, boost::mpl::void_>::type types;

	typedef typename boost::make_variant_over<
		typename boost::mpl::push_back<
		typename boost::mpl::transform<
		types
		, boost::add_pointer<boost::mpl::_>
		>::type
		, boost::blank
		>::type
	>::type variant_type;

	explicit variant_stream()
		: m_variant(boost::blank()) {}

	template <class S>
	void instantiate(S s)
	{
		std::auto_ptr<S> owned(new S(s));
		boost::apply_visitor(delete_visitor(), m_variant);
		m_variant = owned.get();
		owned.release();
	}

	template <class S>
	S* get()
	{
		S** ret = boost::get<S*>(&m_variant);
		if (!ret) return 0;
		return *ret;
	}

	bool instantiated() const
	{
		return m_variant.which() != boost::mpl::size<types>::value;
	}

	~variant_stream()
	{
		boost::apply_visitor(delete_visitor(), m_variant);
	}

	

private:
	variant_type m_variant;
};

}
}

#endif // __SOCKET_TYPE_H__
