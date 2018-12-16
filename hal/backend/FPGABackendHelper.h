#pragma once

#include <sstream>

#include <log4cxx/logger.h>

// hicann-system
#include "reticle_control.h"
#include "hicann_ctrl.h"

#include "hal/Coordinate/iter_all.h"
#include "hal/Handle/FPGAHw.h"
#include "hal/Handle/HICANNHw.h"
#include "hal/backend/FPGABackend.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("halbe.backend.fpga");

namespace HMF {
namespace FPGA {
namespace Helper {

using namespace facets;

void switchramtest_jtag(Handle::FPGAHw& f, ReticleControl& reticle)
{
	size_t const startaddr = 0;
	size_t const maxaddr = 111;
	size_t const datawidth = 16;
	for (auto d : Coordinate::iter_all<HMF::Coordinate::DNCOnFPGA>()) {
		for (auto h : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>()) {
			if (f.hicann_active(d, h)) {
				// JTAG-based access... we need other hicann-system comm objects...
				boost::scoped_ptr<S2C_JtagPhys> s2c_jtag(
				    new S2C_JtagPhys(reticle.comm->getCommAccess(), reticle.jtag.get(), false));
				Stage2Comm* local_comm = static_cast<Stage2Comm*>(s2c_jtag.get());
				HicannCtrl* hc_jtag = new HicannCtrl(local_comm, f.get(d, h)->jtag_addr());
				L1SwitchControl* lc_jtag = &hc_jtag->getLC(HicannCtrl::L1SWITCH_TOP_LEFT);
				if (hc_jtag->GetCommObj()->Init(hc_jtag->addr()) != Stage2Comm::ok)
					throw "reset(): JTAG-based comm init failed";

				// write
				std::vector<size_t> testdatas;
				LOG4CXX_DEBUG(logger, "reset(): Verifying jtag based communication link, write");
				for (size_t i = startaddr; i <= maxaddr; i++) {
					size_t testdata = 0;
					// generate non-zero (almost) random value ;)
					while (testdata == 0)
						testdata = rand() % (1 << datawidth);
					lc_jtag->write_cfg(i, testdata);
					testdatas.push_back(testdata);
				}

				LOG4CXX_DEBUG(logger, "reset(): Verifying jtag based communication link, read");
				// read back and verify
				for (size_t i = startaddr; i <= maxaddr; i++) {
					size_t const testdata = testdatas.at(i);
					lc_jtag->read_cfg(i);
					ci_addr_t rcvaddr = 0;
					ci_data_t rcvdata = 0;
					lc_jtag->get_read_cfg(rcvaddr, rcvdata);
					if (i != rcvaddr || testdata != rcvdata) {
						LOG4CXX_ERROR(logger, "Wrong data read back: "
						                          << std::hex << i << " | " << rcvaddr << " ||| "
						                          << testdata << " | " << rcvdata);
						throw std::runtime_error(
						    "FPGA::reset: JTAG-based write/read access failed");
					}
				}
			}
		}
	}
}

void switchramtest_arq(Handle::FPGAHw& f, ReticleControl& reticle)
{
	size_t const startaddr = 0;
	size_t const maxaddr = 111;
	size_t const datawidth = 16;
	for (auto d : Coordinate::iter_all<HMF::Coordinate::DNCOnFPGA>()) {
		for (auto h : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>()) {
			if (f.hicann_highspeed(d, h)) {
				// Now HICANN-ARQ-based access...
				L1SwitchControl* lc =
				    &reticle.hicann[f.get(d, h)->jtag_addr()]->getLC(HicannCtrl::L1SWITCH_TOP_LEFT);

				// write
				std::vector<size_t> testdatas;
				for (size_t i = startaddr; i <= maxaddr; i++) {
					size_t testdata = 0;
					// generate non-zero (almost) random value ;)
					while (testdata == 0)
						testdata = rand() % (1 << datawidth);
					lc->write_cfg(i, testdata);
					testdatas.push_back(testdata);
				}

				// read back and verify
				for (size_t i = startaddr; i <= maxaddr; i++) {
					size_t const testdata = testdatas.at(i);
					lc->read_cfg(i);
					ci_addr_t rcvaddr = 0;
					ci_data_t rcvdata = 0;
					lc->get_read_cfg(rcvaddr, rcvdata);
					if (i != rcvaddr || testdata != rcvdata) {
						LOG4CXX_ERROR(logger, "Wrong data read back: "
						                          << std::hex << i << " | " << rcvaddr << " ||| "
						                          << testdata << " | " << rcvdata);
						std::stringstream error_msg;
						error_msg << HMF::Coordinate::short_format(f.coordinate())
						          << "::reset: HICANN-ARQ-based write/read access failed";
						throw std::runtime_error(error_msg.str().c_str());
					}
				}
			} else {
				LOG4CXX_INFO(logger, "HMF::FPGA::reset(): Skipping HICANN-ARQ-based switch RAM test on "
					<< HMF::Coordinate::short_format(f.coordinate()) << "/" << d << "/" << h << ".");
			}
		}
	}
}

/**
 * Enforce empty FPGA ingres buffers (mostly HostARQ).
 *
 * This is a *HACK* to provide "blocking" semantics to be used by some FPGA
 * configuration commands. The implementation uses a read from the stats
 * module (HostARQ has FIFO semantics, i.e. everything before the read has
 * to be drained/handled before the read is performed).
 *
 * FIXME: Remove this as soon as the FPGA implements it (i.e. pure "writes"
 * will be "write + wait for ACK").
 */
void wait_for_communication_ingress_buffers_empty(ReticleControl const& reticle)
{
	reticle.fc->get_bitfile_git_hash();
}

} // Helper
} // FPGA
} // HMF
