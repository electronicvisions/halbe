#include "hwtest.h"

#include <iostream>
#include <ctime>

extern "C" {
#include <setjmp.h>
}

#include <boost/make_shared.hpp>

#include "s2c_jtagphys_2fpga.h"
#include "reticle_control.h"

#include "halco/hicann/v2/fwd.h"
#include "halco/common/iter_all.h"

using namespace std;
using namespace facets;
using namespace HMF;

facets::ReticleControl * HWTestHandle< ::HMF::Handle::FPGAHw >::get_reticle()
{
	return hhandle.get_reticle().get();
}

uint8_t HWTestHandle< ::HMF::Handle::FPGAHw >::jtag_addr()
{
	return hhandle.jtag_addr();
}

void HWTestHandle< ::HMF::Handle::FPGAHw >::InstanceSetUp() { // called for every TEST_F
	//::testing::FLAGS_gtest_death_test_style = "threadsafe";
}

void HWTestHandle< ::HMF::Handle::FPGAHw >::InstanceTearDown() {
}

boost::filesystem::path HWTestHandle< ::HMF::Handle::FPGADump >::dump_file =
	boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
