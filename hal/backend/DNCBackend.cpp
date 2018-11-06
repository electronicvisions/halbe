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

HALBE_GETTER(Status, get_dnc_status,
	Handle::FPGA &, f,
	const Coordinate::DNCOnFPGA &, d,
	Coordinate::HICANNOnDNC const&, h)
{
	if (dynamic_cast<Handle::FPGAHw&>(f).isKintex())
		throw std::runtime_error("DNC not available on Kintex");

	ReticleControl& reticle = *f.get_reticle(d);

	Status returnvalue = Status();

	uint8_t crc = 0, status = 0;
	uint64_t id = 0;

	reticle.jtag->read_id(id, reticle.jtag->pos_dnc);

	uint8_t reticle_addr = f.hicann(d,h).toHICANNOnHS().toEnum();
	reticle.jtag->DNC_set_channel(reticle_addr);
	reticle.jtag->DNC_read_crc_count(crc);
	//TODO: not sure about this reset if this is really the HICANN-number
	reticle.jtag->DNC_reset_crc_count(reticle_addr);

	reticle.jtag->DNC_read_channel_sts(reticle_addr, status);

	returnvalue.setCRCCount(crc);
	returnvalue.setStatusReg(status);
	returnvalue.setHardwareId(id);

	return returnvalue;
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
