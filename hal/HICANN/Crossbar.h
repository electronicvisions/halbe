#pragma once

#include <string>

#include "hal/SparseSwitchMatrix.h"
#include "hal/HICANNContainer.h"
#include "halco/hicann/v2/fwd.h"
#include "hal/test.h"

namespace HMF {
namespace HICANN {

class Crossbar :
	public SparseSwitchMatrix<
		Crossbar, bool,
		halco::hicann::v2::VLineOnHICANN,
		halco::hicann::v2::HLineOnHICANN,
		8, 1>
{
public:
	PYPP_CONSTEXPR Crossbar() {}

	static bool exists(x_type x, y_type y);
	std::string check_exclusiveness(size_t max_switches_per_row,
	                                size_t max_switches_per_column) const;

	CrossbarRow const&
	get_row(halco::hicann::v2::HLineOnHICANN y, halco::common::Side s) const;
	void set_row(halco::hicann::v2::HLineOnHICANN y, halco::common::Side s, CrossbarRow const&);

private:
	FRIEND_TEST(HMFConfig, Crossbar);
	FRIEND_TEST(Crossbar, GetSet);
};

} // HICANN
} // HMF
