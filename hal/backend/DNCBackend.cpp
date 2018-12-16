#include "hal/backend/DNCBackend.h"

#include "hal/backend/dispatch.h"
#include "hal/Coordinate/iter_all.h"

// TODO: ugly!
#include "hal/Handle/HMFRun.h"
#include "reticle_control.h"
#include "dnc_control.h"           //DNC control class

using namespace facets;

namespace HMF {
namespace DNC {

HALBE_SETTER(reset,
	Handle::FPGA &, f,
	Coordinate::DNCOnFPGA const &, d)
{
	ReticleControl& reticle = *f.get_reticle(d);

	// ECM: jtag reset requires dnc reset (incl hicann on vertsetup)
	reticle.jtag->FPGA_set_fpga_ctrl(0x1);
}

HALBE_SETTER(set_hicann_directions,
	Handle::FPGA &, f,
	const Coordinate::DNCOnFPGA &, d,
	GbitReticle const&, links)
{
	ReticleControl& reticle = *f.get_reticle(d);

	uint64_t dnccfg=0;    //configuration of DNC-side
	std::bitset<8> dnc_timestamp_enable=0; // timestamp_enable for DNC-side

	//configuration of the DNC-side has to be made for all HICANNs in reticle every time
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) {
		auto link = links[hicann];
		size_t j = hicann.x()*2 + hicann.y(); // convert to numbering on DNC
		std::bitset<8> temp;
		///swap the direction bits according to new coordinates
		for (int i=0; i<8; i++){ //looping over channels
			temp[7-i]=(link.dirs[i]==HMF::HICANN::GbitLink::Direction::TO_HICANN)?true:false;
		}
		dnccfg|=(temp.to_ulong() << (j*8));
		dnc_timestamp_enable[j]=link.timestamp_enable;
	}

	reticle.dc->setTimeStampCtrl(dnc_timestamp_enable.to_ulong());
	reticle.dc->setDirection(dnccfg);
}


HALBE_SETTER(set_loopback,
	Handle::FPGA &, f,
	const Coordinate::DNCOnFPGA &, d,
	Loopback const &, loopback)
{
	ReticleControl& reticle = *f.get_reticle(d);
	std::bitset<8> converted;
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) {
		size_t j = hicann.x()*2 + hicann.y(); // convert to numbering on DNC
		converted[j] = loopback[hicann];
	}
	reticle.jtag->DNC_set_loopback(converted.to_ulong());
}
} // namespace DNC

} //namespace HMF
