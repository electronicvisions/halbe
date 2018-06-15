#pragma once

#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/utility/enable_if.hpp>


#ifndef PYPLUSPLUS
#include <algorithm>
#include <initializer_list>
#define STRONG_TYPEDEF_CONSTRUCTORS(NAME, CONSTEXPR) \
    CONSTEXPR NAME() : type() {} \
	CONSTEXPR NAME(type const& a) : type(a) {} \
	NAME(std::initializer_list<value_type> list) { \
		if (list.size() > type::size()) \
		{ std::__throw_out_of_range(#NAME "::" #NAME "::(initializer_list)"); } \
		std::copy(list.begin(), list.end(), type::begin()); \
	}
#else
#define STRONG_TYPEDEF_CONSTRUCTORS(NAME, CONSTEXPR) \
    CONSTEXPR NAME() : type() {} \
	CONSTEXPR NAME(type const& a) : type(a) {}
#endif

#define STRONG_TYPEDEF_COMMON(NAME) \
	bool operator== (const NAME & o) const { return base() == o.base(); } \
	template<typename T> \
	friend typename boost::enable_if<boost::is_same< T, NAME >, bool>::type \
	operator < (const T & a, const T & b) { return a.base() < b.base(); } \
private: \
	const type & base() const { return *static_cast<const type*>(this); } \
\
	friend class boost::serialization::access; \
	template<typename Archive> \
	void serialize(Archive& ar, unsigned int const x) \
	{ \
		boost::serialization::serialize(ar, *static_cast<type*>(this), x); \
	} \
	friend std::ostream& operator<<(std::ostream& os, NAME const& o);

#define STRONG_TYPEDEF_EMPTY

#define STRONG_TYPEDEF(NAME, TYPE) \
struct NAME : public TYPE { \
	typedef TYPE type; \
	STRONG_TYPEDEF_CONSTRUCTORS(NAME, constexpr) \
	STRONG_TYPEDEF_COMMON(NAME) \
};

#define STRONG_TYPEDEF_CONSTEXPR(NAME, TYPE, CEXPR) \
struct NAME : public TYPE { \
	typedef TYPE type; \
	STRONG_TYPEDEF_CONSTRUCTORS(NAME, CEXPR) \
	STRONG_TYPEDEF_COMMON(NAME) \
};

#define STRONG_TYPEDEF_NO_CONSTEXPR(NAME, TYPE) \
struct NAME : public TYPE { \
	typedef TYPE type; \
	STRONG_TYPEDEF_CONSTRUCTORS(NAME, STRONG_TYPEDEF_EMPTY) \
	STRONG_TYPEDEF_COMMON(NAME) \
};

// This allows to use pywrap::create_constructor with strong typedefs
// Currently supported are: std::arrays

#if !defined(PYPLUSPLUS) && defined(PYBINDINGS)
#include <type_traits>
#include <boost/serialization/array.h>
#include <boost/python/object_fwd.hpp>
#include "pywrap/compat/array.hpp"
#include "pywrap/create_constructor.hpp"

namespace pywrap {
	template<typename T>
	class has_array_base
	{
		private:
			// better match if SFINAE doesn't fail
			typedef char true_type;
			template<typename U, size_t N>
			static typename std::enable_if<
				(!std::is_same<T, std::array<U, N> >::value &&
				 std::is_base_of<std::array<U, N>, T>::value),
				 true_type>::type
			test(std::array<U, N> const*);

			typedef int false_type;
			static false_type test(...);
		public:
			static const bool value =
				(sizeof(test((T const*)NULL))==sizeof(true_type));
	};

	template <typename T>
	struct extract_obj<T,
		typename std::enable_if<has_array_base<T>::value>::type> :
		extract_obj<typename T::type>
	{
	};
}
#endif
