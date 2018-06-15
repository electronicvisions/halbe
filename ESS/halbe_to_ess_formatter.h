#pragma once

#include "hicann_cfg.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "systemsim/HAL2ESSEnum.h"

namespace HMF
{

//functions implemented analog to hicann_cfg_to_halbe_converter (from StHAL)

//converts a halbe VLine to a hicann_cfg/hw vbus
size_t revert_vbus(Coordinate::VLineOnHICANN const& vline);
size_t revert_vbus(size_t const& vline);

//converts a halbe HLine to a hicann_cfg/hw hbus
size_t revert_hbus(Coordinate::HLineOnHICANN const& hline);
size_t revert_hbus(size_t const& hline);

//conversion of synswitchrow
void format_synswitch(Coordinate::SynapseSwitchRowOnHICANN const& s, size_t &synbl, size_t &addr);

//conversion of synapseroe
size_t format_synapse_row(Coordinate::SynapseRowOnHICANN const& s);

//conversion of syndriver coordinates
size_t to_synblock(Coordinate::SynapseDriverOnHICANN const& coord);
size_t to_drvaddr(Coordinate::SynapseDriverOnHICANN const& coord);
    
//conversions of repeater coordinates
ESS::RepeaterLocation to_repblock(Coordinate::RepeaterBlockOnHICANN const& block);
size_t to_repaddr(Coordinate::RepeaterBlockOnHICANN const& block, size_t val);

//conversion of Halbe Merger Coordinates to HW Merger Coordinates
size_t format_merger(size_t const merger);

} //end namespace HMF
