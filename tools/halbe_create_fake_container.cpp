#include <fstream>
#include <boost/archive/text_oarchive.hpp>

#include "hal/HICANNContainer.h"

int main() {

	HMF::HICANN::NeuronConfig nrn_cfg;
	nrn_cfg.activate_neuron_reset();
	nrn_cfg.activate_spl1_reset();

	nrn_cfg.timings = HMF::HICANN::SRAMControllerTimings(
	    HMF::HICANN::SRAMReadDelay(1), HMF::HICANN::SRAMSetupPrecharge(2),
	    HMF::HICANN::SRAMWriteDelay(3));

	std::ofstream nrn_cfg_file{"NeuronConfig_v2.txt"};
	boost::archive::text_oarchive oa_nrn_cfg{nrn_cfg_file};
	oa_nrn_cfg << nrn_cfg;

	//--------------------------------------------------------------------------

	HMF::HICANN::RepeaterBlock repeater_block;

	repeater_block.timings = HMF::HICANN::SRAMControllerTimings(
	    HMF::HICANN::SRAMReadDelay(3), HMF::HICANN::SRAMSetupPrecharge(2),
	    HMF::HICANN::SRAMWriteDelay(1));

	std::ofstream repeater_block_file{"RepeaterBlock_v1.txt"};
	boost::archive::text_oarchive oa_repeater_block{repeater_block_file};
	oa_repeater_block << repeater_block;

}
