#include <gtest/gtest.h>
#include <iostream>

#include "hwtest.h"

#include "reticle_control.h"       //reticle control class

using namespace std;
using namespace geometry;

namespace HMF {

class Layer2InitTest : public ::HWTest {
	// ECM: removed setUp without reset => a testmode has to be standalone!
};


TEST_F(Layer2InitTest, HighspeedInitHWTest) {
	// do repeated init
	size_t const rep_count = 100;
	size_t succ_count = 0;
	for (unsigned int nrep=0; nrep<rep_count; ++nrep) {
		std::bitset<8> hicanns;
		for (unsigned int nhicann=0;nhicann<8;++nhicann)
			hicanns[nhicann] = true;

		bool success = get_reticle()->hicannInit(hicanns, /*silent*/ false, /*return_on_error*/true);
		if (success)
			succ_count += 1;
	}
	EXPECT_EQ(succ_count, rep_count);
}


TEST_F(Layer2InitTest, FPGAResetInitHWTest) {
	// get HICANN handles -> activate all HICANNs on a reticle
	for (int nhicann=0;nhicann<8;++nhicann)
		handle.get(d, HMF::Coordinate::HICANNOnDNC( geometry::Enum(nhicann) ) );

	// do repeated init (via FPGA::reset)
	size_t const rep_count = 100;
	size_t succ_count = 0;
	for (unsigned int nrep=0; nrep<rep_count; ++nrep) {
		try {
			::HMF::FPGA::reset(f);
		} catch(std::runtime_error&) {
			continue;
		}

		succ_count += 1;
	}

	EXPECT_EQ(succ_count, rep_count);
}

} // namespace HMF
