#pragma once

#include <vector>

#include "halco/hicann/v2/fwd.h"
#include "hal/FPGAContainer.h"
//#include "hal/FPGA.h"

#include "RealtimeSpike.h"

namespace HMF {

// fwd decl
namespace Handle{
	class FPGA;
	class HICANN;
}

namespace FPGA {

/**
 * Resets the FPGA-core, DNC links, ARQ and all JTAG-devices in the chain
 */
void reset(Handle::FPGA & f);

/**
 * Resets selected parts of the FPGA.
 */
void reset(Handle::FPGA & f, Reset const& r);

/**
 * Resets host_al's counters for next experiments
 */
void reset_pbmem(Handle::FPGA & f);

/**
 * Init all HICANNs attached to the FPGA
 */
void init(Handle::FPGA & f, bool const zero_synapses=true);

/**
 * Reads out FPGA status register, CRC error register and other status-
 * relevant stuff.
 *
 * @return FPGA status object
 */
Status get_fpga_status(Handle::FPGA & f);

/**
 * Canonical procedure to setup system time counter in FPGA and HICANNs:
 * first "prime" the systime counter for all FPGAs involved. "Prime" includes
 * stopping and resetting the counters for FPGA and HICANNs as well as setting
 * the FPGA to either listen to direct signal from host (listen local mode) or
 * to listen for a signal from the master FPGA (listen global mode). This is
 * decided by a flag in the FPGA handle accessed via getListenGlobal().
 * After this is done for all FPGAs involved in the experiment, call
 * start_systime_counter() with master FPGA (12).
 */
void prime_systime_counter(Handle::FPGA& f);
void disable_global(Handle::FPGA& f);

/**
 * Starts systime counter on FPGA and corresponding HICANNs.
 * In listen global mode function sends signal only if the master FPGA is given.
 * For other FPGAs function just returns.
 */
void start_systime_counter(Handle::FPGA& f);

/**
 * Same behavior as  prime_systime_counter(). First call prime_experiment() which
 * sets up FPGAs to either listen for local or global signal. Then call
 * start_experiment() for at least the master FPGA (12).
 */
void prime_experiment(Handle::FPGA& f);

/**
 * Starts playback and tracing of pulses from/to the FPGA playback and trace memory(DDR2).
 *
 * When FPGA is NOT in listen global mode start signal  is send for each individual FPGA.
 * In listen global mode function sends signal only if the master FPGA is given
 *
 * Playback and trace memory are enabled synchronously, at the next overflow of the FPGA clock counter.
 * The playback is stopped automatically with its last pulse, while tracing always continues.
 * If the playback is run in loop mode, it will never stop (really? see TODO below)
 * Tracing has to be stopped with FPGA::stop_trace_memory(..)
 *
 * TODO(JP):
 *   How to stop playback in loop mode?
 *   How many loops in loop mode? Infinite?
 *   What happens with pulses already in the trace memory? Are those deleted?
 *
 * @param playback_loop = false If true, play pulses from playback memory in a loop,
 * else play pulses only once.
 */
void start_experiment(Handle::FPGA& f);

/**
 * Configures the FPGA backgroung generator
 *
 * The background generator is able to produce poisson-distributed events
 * on 1-8 channels simultaneously with random neuron addresses, but also
 * regular event input on single channel. Only can be used to feed one HICANN
 * at a time!
 *
 * @param bg Configuration data for FPGA BEG
 *
 * @note Seed of the FPGA BEG _MUST_ be !=0 to function properly!
 * @note The rate is somehow multiplied by 2 in the FPGA, so beware!
 */
void set_fpga_background_generator(
	Handle::FPGA & f,
	halco::hicann::v2::DNCOnFPGA const d,
	BackgroundGenerator const& bg);

/**
 * Writes pulses into the FPGA playback memory (DDR2).
 *
 * Pulses are appended to the list of pulses in the DDR2 memory
 *
 * @param st                 Pulse list
 * @param runtime            Experiment runtime in dnc cycles
 * @param fpga_hicann_delay  number of FPGA clk cycles (8ns), by which pulses
 * are released in the FPGA BEFORE the time specified in the FPGAPulseEvents in
 * the pulse list
 * @param enable_trace_recording if true, FPGA records from HICANN
 * @param drop_background_events if true, FPGA does not record L1 events with address 0
 */
void write_playback_program(
    Handle::FPGA& f,
    PulseEventContainer const& st,
    PulseEvent::spiketime_t runtime,
    uint16_t fpga_hicann_delay,
    bool enable_trace_recording,
    bool drop_background_events);

/**
 * Check if end-of-experiment FPGA config packet was acknowledged by FPGA
 * (which indicates that buffering has completed).
 */
bool get_pbmem_buffering_completed(Handle::FPGA & f);

/** 
 * @brief Read pulses from the FPGA trace memory (DDR2).
 * 
 * @param runtime Experiment runtime in dnc cycles
 * @note Before reading the pulses the the trace memory is stopped to make sure that no
 *       new pulses enter the memory during read out.
 */
PulseEventContainer::container_type read_trace_pulses(
    Handle::FPGA& f, PulseEvent::spiketime_t runtime);

/**
*  Set port that the SpiNNaker pulse interface reacts on.
*/
void set_spinnaker_receive_port(
     Handle::FPGA & f,
     uint16_t port
     );

/**
 * Sets SpiNNaker routing table
 * for details see class SpinnRoutingTable
*/
void set_spinnaker_routing_table(
     Handle::FPGA & f,
     SpinnRoutingTable const& spinn_routing_table
     );

/**
 * Set SpiNNaker upsample count.
 * After reset, the upsampler is set to 0 -- thus NO spike will be sent by the FPGA.
*/
void set_spinnaker_pulse_upsampler(
    Handle::FPGA & f,
    size_t upsample_count);

/**
 * Set SpiNNaker downsample count
 */
void set_spinnaker_pulse_downsampler(
     Handle::FPGA & f,
     size_t downsample_count
     );

/**
  * Add pulse to SpiNNaker pulse packet
*/
void add_spinnaker_pulse(
     Handle::FPGA & f,
     SpinnInputAddress_t const& spinn_address
     );

/**
 * Send current list of SpiNNaker pulses to FPGA
*/
void send_spinnaker_pulses(Handle::FPGA & f);
    
/**
 * Get next received SpiNNaker pulse
*/
SpinnOutputAddress_t get_received_spinnaker_pulse(Handle::FPGA & f);



/**
 * Set sending behavior of SpiNNaker IF:
 * see SpinnSenderConfig for details.
 * behavior
 * if cfg.getActive() == true,
 *  the Spinnaker IF sends pulses to the cfg.getTargetIP() on port cfg.getTargetPort()
 * else:
 *  the Spinnaker IF waits untils it receives the first pulse packet, and sends Pulses
 *  to the sender IP and Port.
*/
void set_spinnaker_sender_config(Handle::FPGA & f, SpinnSenderConfig const& cfg);

void set_spinnaker_address_config(Handle::FPGA & f, SpinnAddressConfig const& cfg);

/* TODO: SpiNNaker interface still to implement:
 /// send current list of pulses to FPGA, using specified target port.
 bool sendPulsesToPort(unsigned int pport);
*/
    
/**
 * Use RealtimeComm to send spikes
*/
void send_realtime_pulse(
	Handle::FPGA & f,
	SpinnInputAddress_t spinn_address);

/**
 * Send custom realtime spikes
*/
void send_spinnaker_realtime_pulse(
	Handle::FPGA & f,
	Realtime::spike_h s);
void send_custom_realtime_pulse(
	Handle::FPGA & f,
	Realtime::spike s);

/**
 * Queue realtime spikes to send later or asynchronously by sending thread.
*/
void queue_spinnaker_realtime_pulse(
	Handle::FPGA & f,
	Realtime::spike_h s);

/**
 * Get received realtime spikes
*/
std::vector<SpinnOutputAddress_t> get_received_realtime_pulses(
	Handle::FPGA & f);

/**
 * Wait until next realtime spike is received and return its label
*/
SpinnOutputAddress_t spin_and_get_next_realtime_pulse(
    Handle::FPGA & f);

/**
 * Wait and return next realtime spike (with/without timestamps)
*/
Realtime::spike spin_and_get_next_realtime_pulse_as_custom(
	Handle::FPGA & f);
Realtime::spike_h spin_and_get_next_realtime_pulse_as_spinnaker(
	Handle::FPGA & f);

/**
 * Flush FPGA communication channel
 *
 * This function blocks until Host-FPGA communication channel is idle.
 * The FPGA-Chip communication is not considered in this function.
 * After a maximum waiting time of 500ms a runtime error is thrown.
 *
 */
void flush(Handle::FPGA& f);


} //namespace FPGA
} //namespace HMF
