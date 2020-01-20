#include "hal/HICANN/SynapseSwitch.h"
#include "halco/hicann/v2/synapse.h"

namespace HMF {
namespace HICANN {

bool SynapseSwitch::exists(x_type x, y_type y)
{
	size_t x_mod = x % 32;
	size_t y_mod = y % 16;

	y_mod -= y_mod%2;
	y_mod /= 2;
	x_mod /= 4;

	if(y < y_type::end/2) {
		/* halco::common::TOP */
		return x<128  ? y_mod==x_mod : (7-y_mod)==x_mod;
	} else {
		/* halco::common::BOTTOM */
		return x>=128 ? y_mod==x_mod : (7-y_mod)==x_mod;
	}
}

bool SynapseSwitch::local(x_type x, y_type y) {

	bool const even = (y % 2 == 0);

	// local connects are on the odd lines on the left part and on the even lines on the
	// right
	return x.isLeft() ? (!even) : even;
}

SynapseSwitch::x_lines_for_row_type SynapseSwitch::get_lines(
    halco::hicann::v2::SynapseSwitchRowOnHICANN const& s) {
	x_lines_for_row_type some;

	const x_lines_type all = get_lines(s.line());
	const auto side = s.toSideHorizontal();

	// return only vlines that match the side of the synapse switch row
	std::copy_if(all.begin(), all.end(), some.begin(),
	[&side](halco::hicann::v2::VLineOnHICANN const& v) -> bool {
		return !((v < (halco::hicann::v2::VLineOnHICANN::end/2)) xor (side == halco::common::left));
	});
	return some;
}

/** checks row-wise exclusiveness.
 * checks if for the switches on one side (halco::common::left or RIDE) in each row there is maximum one switch enabled.
 */
std::string SynapseSwitch::check_exclusiveness(
    size_t max_switches_per_row, size_t max_switches_per_column_per_side) const {

	std::stringstream errors;

	// row-wise
	for (size_t yy = 0; yy < y_type::end; ++yy) {
		// halco::common::left
		size_t cnt = 0;
		for (size_t xx = 0; xx < periods * period_length/2; ++xx) {
			cnt += switches()[yy][xx];
		}
		if (cnt > max_switches_per_row) {
			errors << cnt << " switches (ie. more than "
				   << max_switches_per_row
				   << ") enabled on halco::common::left side in row " << yy;
		}
		// halco::common::right
		cnt = 0;
		for (size_t xx = periods * period_length/2; xx < periods * period_length; ++xx) {
			cnt += switches()[yy][xx];
		}
		if (cnt > max_switches_per_row) {
			errors << "SynapseSwitch: " << cnt << " switches (ie. more than "
				   << max_switches_per_row
				   << ") enabled on halco::common::right side in row " << yy;
		}
	}

	// column-wise but left and right separately
	for (size_t xx = 0; xx < x_type::end; ++xx) {

		size_t cnt_left = 0;
		size_t cnt_right = 0;

		for (size_t yy = 0; yy < y_type::end; ++yy) {
			if (exists(x_type(xx), y_type(yy))) {
				if( xx < x_type::end/2 ) {
					cnt_left += get(x_type(xx), y_type(yy));
				} else {
					cnt_right += get(x_type(xx), y_type(yy));
				}
			}
		}

		if (cnt_left > max_switches_per_column_per_side ||
		    cnt_right > max_switches_per_column_per_side) {
			errors << "SynapseSwitch: " << cnt_left << " (left) " << cnt_right
				   << " (right) switches (ie. more than "
				   << max_switches_per_column_per_side << ") in column "
				   << xx;
		}
	}

	return errors.str();

}

/** checks row-wise exclusiveness with config of right neigbour
 * checks if for the switches on one side (halco::common::left or RIDE) in each row there is maximum one switch enabled.
 * @throws std::runtime_error
 */
void SynapseSwitch::check_exclusiveness(const SynapseSwitch & right_neighbour) const
{
	// row-wise
	for (size_t yy = 0; yy < y_type::end; ++yy) {
		size_t cnt = 0;
		// halco::common::right SIDE of this hicann.
		for (size_t xx = periods * period_length/2; xx < periods * period_length; ++xx) {
			cnt += switches()[yy][xx];
		}
		// halco::common::left SIDE of right neighbour hicann.
		halco::hicann::v2::SynapseSwitchRowOnHICANN addr(y_type(yy), halco::common::left);
		SynapseSwitchRow switch_row_nb = right_neighbour.get_row(addr);
		typedef SynapseSwitchRow::iterator SSR_Iter;
		for ( SSR_Iter it = switch_row_nb.begin(); it != switch_row_nb.end(); ++it ) {
			cnt +=  *it;
		}
		if (cnt > 1) {
			std::stringstream error_string;
			error_string << "SynapseSwitch::check_exclusiveness(right_neighbour): more than one switch enabled in row " << yy;
			throw std::runtime_error(error_string.str());
		}
	}
}

SynapseSwitchRow const&
SynapseSwitch::get_row(halco::hicann::v2::SynapseSwitchRowOnHICANN const& drv) const
{
	return reinterpret_cast<SynapseSwitchRow const&>(
		switches()[drv.line()][drv.toSideHorizontal()*4*4]);
}

void SynapseSwitch::set_row(halco::hicann::v2::SynapseSwitchRowOnHICANN const& drv,
			SynapseSwitchRow const& row)
{
	reinterpret_cast<SynapseSwitchRow &>(
		switches()[drv.line()][drv.toSideHorizontal()*4*4]) = row;
}

} // HICANN
} // HMF
