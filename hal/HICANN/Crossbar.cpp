#include "hal/HICANN/Crossbar.h"

#include <sstream>

namespace HMF {
namespace HICANN {

bool Crossbar::exists(x_type x, y_type y)
{
	size_t y_, x_mod = x%32;

	y_ = y - y%2;
	y_ /= 2;

	return x<128 ? (31-y_)==x_mod : y_==x_mod;
}

std::string Crossbar::check_exclusiveness(size_t max_switches_per_row,
                                          size_t max_switches_per_column) const {

	std::stringstream errors;

	// row-wise
	for (size_t yy = 0; yy < y_type::end; ++yy) {
		size_t cnt = 0;
		for (size_t xx = 0; xx < periods * period_length; ++xx) {
			cnt += switches()[yy][xx];
		}
		if (cnt > max_switches_per_row) {
			errors << "Crossbar: " << cnt << " switches (ie. more than "
			       << max_switches_per_row << ") enabled in row " << yy;
		}
	}
	// column-wise
	for (size_t xx = 0; xx < x_type::end; ++xx) {
		size_t cnt = 0;
		for (size_t yy = 0; yy < y_type::end; ++yy) {
			if (exists(x_type(xx), y_type(yy)))
				cnt += get(x_type(xx), y_type(yy));
		}
		if (cnt > max_switches_per_column) {
			errors << "Crossbar: " << cnt << " switches (ie. more than "
			       << max_switches_per_column << ") enabled in column " << xx;
		}
	}

	return errors.str();
}

HICANN::CrossbarRow const&
Crossbar::get_row(Coordinate::HLineOnHICANN y, geometry::Side s) const
{
	return reinterpret_cast<HICANN::CrossbarRow const&>(switches()[y][s*4]);
}

void Crossbar::set_row(
		Coordinate::HLineOnHICANN y, geometry::Side s, CrossbarRow const & row)
{
	reinterpret_cast<HICANN::CrossbarRow&>(switches()[y][s*4]) = row;
}

} // HICANN
} // HMF
