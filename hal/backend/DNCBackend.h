#pragma once
/**
 * This file provides the description of the low level HAL interface for the
 * DNC functionality. Each function itself is stateless.
 * Data needs to be stored and managed on a higher level in the hierarchy.
 */

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/DNCContainer.h"
#include "hal/Handle/FPGA.h"

namespace HMF {
namespace DNC {

/**
 * DNC Design Reset. This triggers HICANN design reset on the vertical setup!
 */
void reset(Handle::FPGA & f, Coordinate::DNCOnFPGA const & d);

/**
 * Sets correct directions for DNC<->HICANN communication in the DNC.
 * DNCControl. Not gettable(TODO:really?)
 *
 * @param d Configuration: directions for all 8 channels of a HICANN
 * @note: For the HICANN side use HICANN::set_gbit_link(..)
 */
void set_hicann_directions(Handle::FPGA & f, const Coordinate::DNCOnFPGA& d, GbitReticle const& links);


/**
 * set loop back mode in the DNC.
 * For each hicann channel in the DNC a loopback can be enabled:
 * The loopback happens after the heap memory.
 * Pulses are routed back via the same hicann channel, such that there is no change in the address.
 * TODO:
 *  - required settings for timestamp control and channel directions?
 *  - does it work w and w/o heap mem respectively delay memory
 *  - gettable?
 *
 * @note: This is not an official feature of the DNC, but rather a debug facility.
 * @note: uses JTAG
 */
void set_loopback(Handle::FPGA & f, const Coordinate::DNCOnFPGA& d, Loopback const & loopback);

} // namespace DNC
} // namespace HMF
