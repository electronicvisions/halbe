#pragma once

#include <string>
#include <sstream>

#include "hal/SparseSwitchMatrix.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANNContainer.h"
#include "hal/test.h"

namespace HMF {
namespace HICANN {

class SynapseSwitch :
	public SparseSwitchMatrix<
		SynapseSwitch, bool,
		Coordinate::VLineOnHICANN,
		Coordinate::SynapseSwitchRowOnHICANN::y_type,
		8, 4>
{
public:
	PYPP_CONSTEXPR SynapseSwitch() {}

	typedef std::array<x_type, periods * period_length / 2>
		x_lines_for_row_type;

	static bool exists(x_type x, y_type y);

	// connects to local or neighbouring HICANN
	static bool local(x_type x, y_type y);

	using SparseSwitchMatrix<
		SynapseSwitch, bool, Coordinate::VLineOnHICANN,
		Coordinate::SynapseSwitchRowOnHICANN::y_type, 8, 4
	>::get_lines;

	// returns VLines only on the side matching the side of the switch row
	static x_lines_for_row_type
	get_lines(Coordinate::SynapseSwitchRowOnHICANN const& s);

	/** checks exclusiveness.
	 * checks if for the switches on one side (geometry::left or right) in each row there
	 * the given maximum number of switches enabled.
	 * checks if for the switches on one side (geometry::left or right) in each column
	 * there the given maximum number of switches enabled.
	 */
	std::string check_exclusiveness(size_t max_switches_per_row,
	                                size_t max_switches_per_column_per_side) const;

	/** checks row-wise exclusiveness with config of right neigbour
	 * checks if for the switches on one side (geometry::left or RIDE) in each row there is maximum one switch enabled.
	 * @throws std::runtime_error
	 */
	void check_exclusiveness(const SynapseSwitch & right_neighbour) const;

	SynapseSwitchRow const&
	get_row(Coordinate::SynapseSwitchRowOnHICANN const& drv) const;
	void set_row(Coordinate::SynapseSwitchRowOnHICANN const& drv,
			SynapseSwitchRow const& row);

private:
	FRIEND_TEST(HMFConfig, SynapseSwitch);
	FRIEND_TEST(SynapseSwitch, GetSet);
};

} // HICANN
} // HMF
