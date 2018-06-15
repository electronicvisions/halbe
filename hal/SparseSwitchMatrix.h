#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>
#include <cassert>
#include <iomanip>
#include <ostream>
#include <string>
#include <sstream>

#include "pywrap/compat/array.hpp"
#include "pywrap/compat/debug.hpp"
#include "pywrap/compat/macros.hpp"

namespace HMF {

/// exception type
template <typename x_type, typename y_type>
struct InvalidSwitch : std::runtime_error {
	static std::string convert(x_type const& x, y_type const& y) {
		std::stringstream s;
		s << "(" << x << ", " << y << ")";
		return s.str();
	}

	InvalidSwitch(x_type const& x, y_type const& y) : std::runtime_error(convert(x, y)) {}
};

template<typename Derived, typename SwitchType,
	typename XType, typename YType,
	size_t Periods, size_t PeriodLength>
class SparseSwitchMatrix
{
public:
	typedef SwitchType value_type;
	typedef XType x_type;
	typedef YType y_type;

	typedef std::array<x_type, Periods * PeriodLength>
		x_lines_type;

	static const size_t periods = Periods;
	static const size_t period_length = PeriodLength;

	PYPP_CONSTEXPR SparseSwitchMatrix();

	value_type get(x_type x, y_type y) const;
	void       set(x_type x, y_type y, value_type v);

	void clear();

	/**
	 * @brief returns an array containing all valid x coordinates for given @param y
	 *
	 * @note This can be implemented / shadowed in Derived using arithmetic
	 *		 to provide better performance.
	 */
	static x_lines_type get_lines(y_type const& y);

	static bool exists(x_type x, y_type y);

	bool operator==(SparseSwitchMatrix const& a) const;
	bool operator!=(SparseSwitchMatrix const& a) const;

	friend std::ostream& operator<< (std::ostream& os, Derived const& a)
	{
		os << ZTL::typestring<Derived>() << ":" << std::endl;
		return os << static_cast<SparseSwitchMatrix const&>(a);
	}

	friend std::ostream& operator<< (std::ostream& os, SparseSwitchMatrix const& a)
	{
		typedef typename SparseSwitchMatrix::x_type xt;
		typedef typename SparseSwitchMatrix::y_type yt;
		os << "\t\t" << std::hex;
		size_t x=0;
		for (size_t xx=0; xx<xt::end; ++xx) {
			if (a.exists(xt(xx), yt(0))) { // assume rect. shape -> eval "exists" at y=0
				os << x++%16;
			}
		}
		os << '\n' << std::dec;
		for (size_t yy=0; yy<yt::end; ++yy)
		{
			os << "\t" << yy << "\t";
			for (size_t xx=0; xx<xt::end; ++xx)
			{
				if (a.exists(xt(xx), yt(yy)))
					os << (a.get(xt(xx), yt(yy)) ? "X" : ".");
			}
			os << std::endl;
		}
		return os;
	}

protected:
	// TODO: in next release `row_type` could become bitset
	typedef std::array<value_type, periods * period_length> row_type;
	typedef std::array<row_type, y_type::end>               matrix_type;

	matrix_type&       switches();
	matrix_type const& switches() const;

	value_type&       _get(x_type x, y_type y);
	value_type const& _get(x_type x, y_type y) const;

private:
	matrix_type mSwitches;

	//template<typename T> friend class Backend;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};

} // HMF


// implementations

#define SPARSE_SWITCH_HEADER \
	template<typename Derived, typename SwitchType, \
		typename XType, typename YType, \
		size_t Periods, size_t PeriodLength>

#define SPARSE_SWITCH_TYPE \
	SparseSwitchMatrix< \
		Derived, SwitchType, XType, YType, \
		Periods, PeriodLength>

namespace HMF {

SPARSE_SWITCH_HEADER
PYPP_CONSTEXPR SPARSE_SWITCH_TYPE::SparseSwitchMatrix() :
	mSwitches()
{}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::value_type
SPARSE_SWITCH_TYPE::get(x_type x, y_type y) const
{
	return _get(x, y);
}

SPARSE_SWITCH_HEADER
void SPARSE_SWITCH_TYPE::set(x_type x, y_type y, value_type v)
{
	_get(x, y) = v;
}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::x_lines_type
SPARSE_SWITCH_TYPE::get_lines(y_type const& y)
{
	size_t idx = 0;
	x_lines_type lines;

	for (size_t ii = 0; ii < x_type::end; ++ii)
	{
		x_type x(ii);
		if (exists(x, y)) {
			lines[idx++] = x;
		}
	}

	assert(idx == lines.size());

	return lines;
}

SPARSE_SWITCH_HEADER
bool SPARSE_SWITCH_TYPE::exists(x_type x, y_type y)
{
	return Derived::exists(x, y);
}

SPARSE_SWITCH_HEADER
bool SPARSE_SWITCH_TYPE::operator==(SPARSE_SWITCH_TYPE const& a) const
{
	return switches() == a.switches();
}

SPARSE_SWITCH_HEADER
bool SPARSE_SWITCH_TYPE::operator!=(SPARSE_SWITCH_TYPE const& a) const
{
	return switches() != a.switches();
}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::matrix_type&
SPARSE_SWITCH_TYPE::switches()
{
	return mSwitches;
}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::matrix_type const&
SPARSE_SWITCH_TYPE::switches() const
{
	return mSwitches;
}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::value_type&
SPARSE_SWITCH_TYPE::_get(x_type x, y_type y)
{
	SparseSwitchMatrix const& This = *this;
	return const_cast<value_type&>(This._get(x, y));
}

SPARSE_SWITCH_HEADER
typename SPARSE_SWITCH_TYPE::value_type const&
SPARSE_SWITCH_TYPE::_get(x_type x, y_type y) const
{
	if (!exists(x, y)) {
		throw InvalidSwitch<x_type, y_type>(x, y);
	}
	return switches()[y][x/(x_type::end/periods)*period_length + x%period_length];
}

SPARSE_SWITCH_HEADER
void
SPARSE_SWITCH_TYPE::clear()
{
#ifndef PYPLUSPLUS
	for (auto & row : mSwitches)
	{
		std::fill(row.begin(), row.end(), value_type());
	}
#endif
}

SPARSE_SWITCH_HEADER
template<typename Archiver>
void SPARSE_SWITCH_TYPE::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("switches", mSwitches);
}

SPARSE_SWITCH_HEADER
size_t const SPARSE_SWITCH_TYPE::periods;

SPARSE_SWITCH_HEADER
size_t const SPARSE_SWITCH_TYPE::period_length;

} // HMF

#undef SPARSE_SWITCH_HEADER
#undef SPARSE_SWITCH_TYPE
