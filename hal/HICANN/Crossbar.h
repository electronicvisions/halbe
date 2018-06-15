#pragma once

#include <string>

#include "hal/SparseSwitchMatrix.h"
#include "hal/HICANNContainer.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/test.h"

namespace HMF {
namespace HICANN {

class Crossbar :
	public SparseSwitchMatrix<
		Crossbar, bool,
		Coordinate::VLineOnHICANN,
		Coordinate::HLineOnHICANN,
		8, 1>
{
public:
	PYPP_CONSTEXPR Crossbar() {}

	static bool exists(x_type x, y_type y);
	std::string check_exclusiveness(size_t max_switches_per_row,
	                                size_t max_switches_per_column) const;

	CrossbarRow const&
	get_row(Coordinate::HLineOnHICANN y, geometry::Side s) const;
	void set_row(Coordinate::HLineOnHICANN y, geometry::Side s, CrossbarRow const&);

private:
	FRIEND_TEST(HMFConfig, Crossbar);
	FRIEND_TEST(Crossbar, GetSet);
};

} // HICANN
} // HMF
