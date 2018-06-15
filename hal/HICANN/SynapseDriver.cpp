#include "hal/HICANN/SynapseDriver.h"

using namespace HMF::Coordinate;

namespace HMF {
namespace HICANN {

size_t const
	SynapseDriver::num_rows,
	SynapseDriver::num_cap;

bool const
	SynapseDriver::FACILITATION,
	SynapseDriver::DEPRESSION;

void SynapseDriver::disable()           { enable = false; }
void SynapseDriver::set_l1()            { enable = true; locin  = true;  connect_neighbor = false; }
void SynapseDriver::set_l1_mirror()     { enable = true; locin  = true;  connect_neighbor = true; }
void SynapseDriver::set_mirror()        { enable = true; locin  = false; connect_neighbor = true; }
void SynapseDriver::set_mirror_only()   { enable = false;locin  = false; connect_neighbor = true; }
void SynapseDriver::set_listen()        { enable = true; locin  = false; connect_neighbor = false; }

bool SynapseDriver::is_enabled() const      { return enable; }
bool SynapseDriver::is_l1() const           { return enable  &&  locin && !connect_neighbor; }
bool SynapseDriver::is_l1_mirror() const    { return enable  &&  locin &&  connect_neighbor; }
bool SynapseDriver::is_mirror() const       { return enable  && !locin &&  connect_neighbor; }
bool SynapseDriver::is_mirror_only() const  { return !enable && !locin &&  connect_neighbor; }
bool SynapseDriver::is_listen() const       { return enable  && !locin && !connect_neighbor; }

void SynapseDriver::set_stf()       { stp_enable = true; stp_mode = FACILITATION; }
void SynapseDriver::set_std()       { stp_enable = true; stp_mode = DEPRESSION; }
void SynapseDriver::disable_stp()   { stp_enable = false; }
bool SynapseDriver::is_stf() const  { return stp_enable && stp_mode == FACILITATION; }
bool SynapseDriver::is_std() const  { return stp_enable && stp_mode == DEPRESSION; }

bool SynapseDriver::operator==(SynapseDriver const& b) const
{
	return enable == b.enable
		&& connect_neighbor == b.connect_neighbor
		&& locin == b.locin
		&& stp_enable == b.stp_enable
		&& stp_mode == b.stp_mode
		&& stp_cap == b.stp_cap
		&& mRowConfig == b.mRowConfig;
}

bool SynapseDriver::operator!=(SynapseDriver const& b) const
{
	return !(*this==b);
}

RowConfig const& SynapseDriver::operator[] (RowOnSynapseDriver const& r) const
{
	return mRowConfig[r];
}

RowConfig& SynapseDriver::operator[] (RowOnSynapseDriver const& r)
{
	return mRowConfig[r];
}

std::ostream& operator<<(std::ostream& os, const HMF::HICANN::SynapseDriver & m)
{
	os << "SynapseDriver: Enable: " << m.enable << ", Connect Neighbor: " << m.connect_neighbor <<
	", L1 Input: " << m.locin << ", STP Enable: " << (m.stp_enable ? "true" : "false") << ", STP Mode: " << (m.stp_mode ? "true" : "false") <<
	", STP Capacitance: " << m.stp_cap <<
	",\nTop Row Configuration:    " << m.mRowConfig[0] << 
	",\nBottom Row Configuration: " << m.mRowConfig[1];
	return os;
}

} // HICANN
} // HMF
