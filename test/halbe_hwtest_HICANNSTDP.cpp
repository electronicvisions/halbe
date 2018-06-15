#include <gtest/gtest.h>

#include <iostream>
#include <time.h>
#include <fstream>
#include <bitter/bitset.h>
#include <logger.h>

#include "hwtest.h"

#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

using std::cout;
using std::endl;
using std::flush;
using std::make_pair;

namespace HMF {
#define RET reticle.hicann[myPowerBackend.hicann_jtag_addr(h)]
#define HCL1 facets::HicannCtrl::L1Switch
#define HCFG facets::HicannCtrl::FG
#define HCREP facets::HicannCtrl::Repeater
#define HCSYN facets::HicannCtrl::Synapse

using namespace HICANN;

class HICANNSTDPTest : public ::HWTest {
public:
	HICANNSTDPTest()
		:	HWTest(),
			log(Logger::instance()) {
		for(int i=0; i<16; i++) {
			inc[i] = std::min(i+1, 15);
			dec[i] = std::max(i-1,  0);
			nop[i] = i;
			lowest[i] = 0;
			highest[i] = 15;
		}
	}

protected:
	Logger& log;
	STDPLUT::LUT inc, dec, nop, lowest, highest;
	HICANN::GbitLink gl;
	DNC::GbitReticle gr;
	HICANN::FGControl fgc;
	STDPAnalog stdparam;
	STDPControl stdp;


	void set_cmp_voltages(uint16_t const& vth, uint16_t const& vtl, HICANN::FGControl& fgc, STDPAnalog& stdparam) {
		if( (vth != stdparam.V_thigh) || (vtl != stdparam.V_tlow) ) {
			stdparam.V_thigh = vth;
			stdparam.V_tlow = vtl;
			// FIXME: reinstate stdp stuff
			//fgc.load_stdp_parameters(top, stdparam); //load STDP parameters to the floating gates
			//fgc.load_stdp_parameters(bottom, stdparam); //load STDP parameters to the floating gates

			//HICANN::set_PLL_frequency(h, 100);
			for(uint8_t i=0; i<4; i++) {
				Coordinate::FGBlockOnHICANN b{Enum{i}};
				HICANN::set_fg_config(h, b, HICANN::FGConfig());
				fgc.setShared(b, HICANN::shared_parameter::V_thigh, vth);
				fgc.setShared(b, HICANN::shared_parameter::V_tlow, vtl);
				HICANN::set_fg_values(h, b, fgc.getBlock(b));
				HICANN::set_fg_values(h, b, fgc.getBlock(b));
			}
			//HICANN::set_PLL_frequency(h, 150);
		}
	}

	void stdp_cycle(STDPControl& stdp, SideVertical const & side, STDPLUT::LUT const& causal, STDPLUT::LUT const& acausal, STDPLUT::LUT const& combined) {
		stdp.continuous_autoupdate = 0;
		stdp.set_first_row(0);
		stdp.set_last_row(223);
		stdp.lut.causal = causal;
		stdp.lut.acausal = acausal;
		stdp.lut.combined = combined;

		HICANN::set_stdp_config(h, side, stdp);
		HICANN::start_stdp(h, side, stdp);
		HICANN::flush(h);
		usleep(100000);  // does not work without this usleep // ECM: comment exists since before synching correctly
		HICANN::wait_stdp(h, side, stdp);
		HICANN::stop_stdp(h, side, stdp);
	}

	void minimal_init(uint16_t const& vth, uint16_t const& vtl) {
		static bool resetted = false;

		log(Logger::INFO) << "Minimal initialization...";

		if( !resetted ) {
			FPGA::reset(f);
			//HICANN::reset(h);
			//resetted = true;
		}
		HICANN::init(h, false);

		fgc = HICANN::FGControl();
		HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::FGConfig());
		HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(1)), HICANN::FGConfig());
		HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(2)), HICANN::FGConfig());
		HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(3)), HICANN::FGConfig());

		for (size_t blocknr = 0; blocknr < 4; blocknr++)
		{
			auto block = Coordinate::FGBlockOnHICANN(Enum(blocknr));
			fgc.setShared(block, HICANN::shared_parameter::I_breset, 1023);
			fgc.setShared(block, HICANN::shared_parameter::I_bstim, 1023);
			fgc.setShared(block, HICANN::shared_parameter::int_op_bias, 1023);
			fgc.setShared(block, HICANN::shared_parameter::V_br, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_bstdf, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_ccas, 800);
			fgc.setShared(block, HICANN::shared_parameter::V_dep, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_dllres, 200);
			fgc.setShared(block, HICANN::shared_parameter::V_dtc, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_fac, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax0, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax1, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax2, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax3, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_m, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_reset, 450);
			fgc.setShared(block, HICANN::shared_parameter::V_stdf, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_thigh, vth);
			fgc.setShared(block, HICANN::shared_parameter::V_tlow, vtl);
			if (blocknr == 0 || blocknr == 2) {
				fgc.setShared(block, HICANN::shared_parameter::V_bout, 1023);//1023);
				fgc.setShared(block, HICANN::shared_parameter::V_clra, 0);
			} else {
				fgc.setShared(block, HICANN::shared_parameter::V_bexp, 0); // 1023
				fgc.setShared(block, HICANN::shared_parameter::V_clrc, 0);
			}
		}

		for(uint8_t i=0; i<4; i++) {
			Coordinate::FGBlockOnHICANN b{Enum{i}};
			HICANN::set_fg_values(h, b, fgc.getBlock(b));
		}
	}

	void common_init() {
		FPGA::reset(f);
		//HICANN::reset(h, 150);
		//HICANN::reset(h);
		HICANN::init(h, false);

		fgc = HICANN::FGControl();

		//FG parameters for neurons just under the threshold (almost spiking)
		for (size_t neuron_number = 0; neuron_number < 128; neuron_number++)
		{
			Coordinate::NeuronOnHICANN nrn{Enum(neuron_number)};
			fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 450);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 150);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 796);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_bexp, 0);//1023);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convi, 1023);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convx, 1023);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 0);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 100);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbi, 800);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbx, 800);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 100);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 0);//511);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 0);//300);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::I_spikeamp, 1023);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 0);//500);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syni, 350);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 820);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 825);//v_syntcx);
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_synx, 568);
			//fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 500); //threshold neuron0
			fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 800); //threshold neuron0
		}

		for (size_t blocknr = 0; blocknr < 2; blocknr++)
		{
			auto block = Coordinate::FGBlockOnHICANN(Enum(blocknr));
			fgc.setShared(block, HICANN::shared_parameter::I_breset, 1023);
			fgc.setShared(block, HICANN::shared_parameter::I_bstim, 1023);
			fgc.setShared(block, HICANN::shared_parameter::int_op_bias, 1023);
			fgc.setShared(block, HICANN::shared_parameter::V_br, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_bstdf, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_ccas, 800);
			fgc.setShared(block, HICANN::shared_parameter::V_dep, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_dllres, 200);
			fgc.setShared(block, HICANN::shared_parameter::V_dtc, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_fac, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax0, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax1, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax2, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_gmax3, 1000);
			fgc.setShared(block, HICANN::shared_parameter::V_m, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_reset, 450);
			fgc.setShared(block, HICANN::shared_parameter::V_stdf, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_thigh, 0);
			fgc.setShared(block, HICANN::shared_parameter::V_tlow, 0);
			if (blocknr == 0 || blocknr == 2) {
				fgc.setShared(block, HICANN::shared_parameter::V_bout, 1023);//1023);
				fgc.setShared(block, HICANN::shared_parameter::V_clra, 0);
			} else {
				fgc.setShared(block, HICANN::shared_parameter::V_bexp, 0); // 1023
				fgc.setShared(block, HICANN::shared_parameter::V_clrc, 0);
			}
		}

		stdparam = STDPAnalog();
		// FIXME:
		//fgc.load_stdp_parameters(top, stdparam); //load STDP parameters to the floating gates

		//HICANN::set_PLL_frequency(h, 100);
		{
			Coordinate::FGBlockOnHICANN b {Enum{0}};
			HICANN::set_fg_config(h, b, HICANN::FGConfig());
			HICANN::set_fg_values(h, b, fgc.getBlock(b)); //write 2 times for better accuracy
		}
		{
			Coordinate::FGBlockOnHICANN b {Enum{1}};
			HICANN::set_fg_config(h, b, HICANN::FGConfig());
			HICANN::set_fg_values(h, b, fgc.getBlock(b));
		}
		//HICANN::set_PLL_frequency(h, 150);

		// configure DNC mergers
		HICANN::DNCMergerLine mergers;
		HICANN::DNCMerger mer;
		for (size_t  i = 0; i < 8; i++){
			mer.config = HICANN::Merger::MERGE; //syndriver locking comes from HICANN BEG, input - from L2
			mer.slow = true;
			mer.loopback = false;
			mergers[DNCMergerOnHICANN(i)] = mer;
		}
		HICANN::set_dnc_merger(h, mergers);

		// configure merger tree, phase
		HICANN::MergerTree tree = HICANN::MergerTree();
		HICANN::Phase phase = 0;
		HICANN::set_merger_tree(h, tree);
		HICANN::set_phase(h, phase);

		// configure Background Generator
		HICANN::BackgroundGeneratorArray bgarray;
		HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();

		uint16_t period = 1000;
		bg.enable(true);
		bg.random(false);
		bg.seed(200);
		bg.period(period);
		bg.address(HICANN::L1Address(0));

		//all BEGs send locking packets
		for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
		HICANN::set_background_generator(h, bgarray);

		// Enable Repeater
		HICANN::set_repeater_block(h, Coordinate::RepeaterBlockOnHICANN(X(0), Y(1)), HICANN::RepeaterBlock());

		HICANN::HorizontalRepeater sr;
		// enable output on this hicann
		sr.setOutput(right);

		Coordinate::DNCMergerOnHICANN bg7(7), bg0(0);
		Coordinate::SendingRepeaterOnHICANN bg7rep = bg7.toSendingRepeaterOnHICANN();
		Coordinate::SendingRepeaterOnHICANN bg0rep = bg0.toSendingRepeaterOnHICANN();

		ASSERT_EQ(Coordinate::HLineOnHICANN( 6), bg7rep.toHRepeaterOnHICANN().toHLineOnHICANN());
		ASSERT_EQ(Coordinate::HLineOnHICANN(62), bg0rep.toHRepeaterOnHICANN().toHLineOnHICANN());
		HICANN::set_repeater(h, bg7rep.toHRepeaterOnHICANN(), sr); //BG7, channel 7 -> syndriver 111 (STDP synapse)
		HICANN::set_repeater(h, bg0rep.toHRepeaterOnHICANN(), sr); //BG0, channel 0 -> syndriver 97 (strong non-STDP synapse)

		/*// Crossbar Switch
		Crossbar cb;
		HICANN::crossbar_row_t row_cfg;

		cb.set(XLocal(28), YLocal(6), true); //BG7, channel 7 -> syndriver 111
		row_cfg = cb.get_row(HWire(6), geometry::left);
		HICANN::set_crossbar_switch_row(h, HWire(6), geometry::left, row_cfg);

		cb.set(XLocal(0), YLocal(62), true); //BG0, channel 0 -> syndriver 97
		row_cfg =  cb.get_row(HWire(62), geometry::left);
		HICANN::set_crossbar_switch_row(h, HWire(62), geometry::left, row_cfg);

		// Syndriver Switch
		SynapseSwitch sw;
		HICANN::synapseswitch_row_t srow_cfg;
		Coordinate::SynapseDriverSwitchRow addr(Coordinate::SynapseDriverSwitchRow(EnumLocal(111), geometry::left));

		sw.set(XLocal(28), YLocal(111), true); //BG7, channel 7 -> syndriver 111
		srow_cfg = sw.get_row(addr);
		HICANN::set_syndriver_switch_row(h, addr, srow_cfg);

		sw.set(XLocal(0), YLocal(97), true); //BG0, channel 0 -> syndriver 97
		Coordinate::SynapseDriverSwitchRow addr1(Coordinate::SynapseDriverSwitchRow(EnumLocal(97), geometry::left));
		srow_cfg = sw.get_row(addr1);
		HICANN::set_syndriver_switch_row(h, addr1, srow_cfg);

		// Synapse Driver
		SynapseArrayRow sar;
		auto driver = sar.getDriver();
		driver.set_l1();
		sar.setDriver(driver);
		sar.config[SynapseArrayRow::top].decoder[LineConfig::left] = 0;
		sar.config[SynapseArrayRow::top].decoder[LineConfig::right] = 0;
		sar.config[SynapseArrayRow::top].gmax_div[left] = 1;
		sar.config[SynapseArrayRow::top].gmax_div[right] = 1;
		sar.config[SynapseArrayRow::top].syn_in[left] = 1;
		sar.config[SynapseArrayRow::top].syn_in[right] = 0;
		sar.config[SynapseArrayRow::top].selgmax = 0;
		sar.config[SynapseArrayRow::bottom].decoder[LineConfig::left] = 0;
		sar.config[SynapseArrayRow::bottom].decoder[LineConfig::right] = 0;
		sar.config[SynapseArrayRow::bottom].gmax_div[left] = 1;
		sar.config[SynapseArrayRow::bottom].gmax_div[right] = 1;
		sar.config[SynapseArrayRow::bottom].syn_in[left] = 1;
		sar.config[SynapseArrayRow::bottom].syn_in[right] = 0;
		sar.config[SynapseArrayRow::bottom].selgmax = 0;
		// set_synapse_driver includes toggling the drivers DLL
		HICANN::set_synapse_driver(h, Coordinate::SynapseDriver(geometry::EnumLocal(111), geometry::left), sar); //BG7, channel 7 -> syndriver 111
		HICANN::set_synapse_driver(h, Coordinate::SynapseDriver(geometry::EnumLocal(97), geometry::left), sar); //BG0, channel 0 -> syndriver 97

		// Synapses
		HICANN::decoder_drow_t drow;
		HICANN::weight_row_t row;
		std::fill(drow[0].begin(), drow[0].end(), SynapseDecoder(0xf)); //initialize with blocking values
		std::fill(drow[1].begin(), drow[1].end(), SynapseDecoder(0xf));
		std::fill(row.begin(), row.end(), SynapseWeight(0x7)); //initialize with lowest values

		//STDP synapse, initial weight 7, decoder 1
		drow[0][0] = 1;
		HICANN::set_decoder_double_row(h, Coordinate::SynapseDriver(EnumLocal(111), geometry::left), drow);
		HICANN::set_weights_row(h, Coordinate::SynapseDriver(EnumLocal(111), geometry::left), false, row); //top line of a driver
		//row[0] = 7; //only one synapse (lower)
		HICANN::set_weights_row(h, Coordinate::SynapseDriver(EnumLocal(111), geometry::left), true, row); //bottom line of a driver

		//non-STDP synapse (2 synapses combined), max. weight, decoder 2
		drow[0][0] = 2;
		drow[1][0] = 2;
		row[0] = 15;
		HICANN::set_decoder_double_row(h, Coordinate::SynapseDriver(EnumLocal(97), geometry::left), drow);
		HICANN::set_weights_row(h, Coordinate::SynapseDriver(EnumLocal(97), geometry::left), false, row); //top line of a driver
		HICANN::set_weights_row(h, Coordinate::SynapseDriver(EnumLocal(97), geometry::left), true, row); //bottom line of a driver

		// Neurons
		NeuronConfig nconf = NeuronConfig(); //default should be OK
		HICANN::set_neuron_config(h, nconf);

		Neuron neu = Neuron();
		NeuronQuad nquad = NeuronQuad();

		neu.address(HICANN::L1Address(32)); //not to get mixed up
		neu.activate_firing(true);
		neu.enable_fire_input(false);
		neu.enable_aout(true);

		nquad[Coordinate::QuadNeuron(XLocal(0), YLocal(0))] = neu; //only top left neuron fires
		neu.enable_aout(false);
		neu.activate_firing(false);
		nquad[Coordinate::QuadNeuron(XLocal(1), YLocal(0))] = neu;
		nquad[Coordinate::QuadNeuron(XLocal(0), YLocal(1))] = neu;
		nquad[Coordinate::QuadNeuron(XLocal(1), YLocal(1))] = neu;

		nquad.setVerticalInterconnect(XLocal(0), false);
		nquad.setVerticalInterconnect(XLocal(1), false);
		nquad.setHorizontalInterconnect(YLocal(0), false);
		nquad.setHorizontalInterconnect(YLocal(1), false);

		HICANN::set_denmem_quad(h, HMF::Coordinate::NeuronBlock(0), HMF::Coordinate::NeuronQuad(0), nquad);

		// FPGA FIFO input
		//HICANN::GbitLink gl;
		gl.dirs[0] = HICANN::GbitLink::Direction::TO_HICANN;
		gl.dirs[7] = HICANN::GbitLink::Direction::TO_HICANN;

		//GbitReticle gr;
		gr[0] = gl;

		HICANN::set_gbit_link(h, gl);
		DNC::set_hicann_directions(f.dnc(d), gr);*/

	// Crossbar Switch
	Crossbar cb;
	HICANN::CrossbarRow row_cfg;

	cb.set(Coordinate::VLineOnHICANN(28), bg7rep.toHLineOnHICANN(), true); //BG7, channel 7 -> syndriver 111
	row_cfg = cb.get_row(bg7rep.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg7rep.toHLineOnHICANN(), left, row_cfg);

	cb.set(Coordinate::VLineOnHICANN(0), bg0rep.toHLineOnHICANN(), true); //BG0, channel 0 -> syndriver 97
	row_cfg =  cb.get_row(bg0rep.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg0rep.toHLineOnHICANN(), left, row_cfg);

	// Syndriver Switch
	SynapseSwitch sw;
	HICANN::SynapseSwitchRow srow_cfg;
	Coordinate::SynapseSwitchRowOnHICANN addr(Coordinate::SynapseSwitchRowOnHICANN(Y(111), left));

	sw.set(Coordinate::VLineOnHICANN(28), addr.line(), true); //BG7, channel 7 -> syndriver 111
	srow_cfg = sw.get_row(addr);
	HICANN::set_syndriver_switch_row(h, addr, srow_cfg);

	Coordinate::SynapseSwitchRowOnHICANN addr1(Coordinate::SynapseSwitchRowOnHICANN(Y(97), left));

	sw.set(Coordinate::VLineOnHICANN(0), addr1.line(), true); //BG0, channel 0 -> syndriver 97
	srow_cfg = sw.get_row(addr1);
	HICANN::set_syndriver_switch_row(h, addr1, srow_cfg);

	// Synapse Driver
	HICANN::SynapseDriver driver;
	driver.set_l1();
	driver[top].set_decoder(top, DriverDecoder(0));
	driver[top].set_decoder(bottom, DriverDecoder(0));
	driver[top].set_gmax_div(left, 1);
	driver[top].set_gmax_div(right, 1);
	driver[top].set_syn_in(left, 1);
	driver[top].set_syn_in(right, 0);
	driver[top].set_gmax(0);
	driver[bottom] = driver[top];
	// set_synapse_driver includes toggling the drivers DLL
	HICANN::set_synapse_driver(h, SynapseDriverOnHICANN(Y(111), left), driver); //BG7, channel 7 -> syndriver 111
	HICANN::set_synapse_driver(h, SynapseDriverOnHICANN(Y(97), left), driver); //BG0, channel 0 -> syndriver 97

	// Synapses
	HICANN::DecoderDoubleRow drow;
	HICANN::WeightRow row;
	std::fill(drow[0].begin(), drow[0].end(), SynapseDecoder(0xf)); //initialize with blocking values
	std::fill(drow[1].begin(), drow[1].end(), SynapseDecoder(0xf));
	std::fill(row.begin(), row.end(), SynapseWeight(15)); //0x7)); //initialize with lowest values

	//STDP synapse, initial weight 7, decoder 1
	drow[0][0] = SynapseDecoder(0);//1;

	{
		SynapseDriverOnHICANN drv{Y(111), left};
		HICANN::set_decoder_double_row(h, drv, drow);
		HICANN::set_weights_row(h, SynapseRowOnHICANN(drv, top), row); //top line of a driver
		//row[0] = 7; //only one synapse (lower)
		HICANN::set_weights_row(h, SynapseRowOnHICANN(drv, bottom), row); //bottom line of a driver
	}

	//non-STDP synapse (2 synapses combined), max. weight, decoder 2
	drow[0][0] = SynapseDecoder(0); //2;
	drow[1][0] = SynapseDecoder(0); //2;
	//row[0] = 15;
	{
		SynapseDriverOnHICANN drv{Y(97), left};
		HICANN::set_decoder_double_row(h, drv, drow);
		HICANN::set_weights_row(h, SynapseRowOnHICANN(drv, top), row); //top line of a driver
		HICANN::set_weights_row(h, SynapseRowOnHICANN(drv, bottom), row); //bottom line of a driver
	}

	//for(int i=0; i<112; i++) {
	//	if( i % 2 == 0 ) {
	//		HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(i), right, top), row); //top line of a driver
	//		HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(i), right, bottom), row); //bottom line of a driver
	//	} else {
	//		HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(i), left, top), row); //top line of a driver
	//		HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(i), left, bottom), row); //bottom line of a driver
	//	}
	//}


	// Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(32)); //not to get mixed up
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(true);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu; //only top left neuron fires
	neu.enable_aout(false);
	neu.activate_firing(false);
	nquad[Coordinate::NeuronOnQuad(X(1), Y(0))] = neu;
	nquad[Coordinate::NeuronOnQuad(X(0), Y(1))] = neu;
	nquad[Coordinate::NeuronOnQuad(X(1), Y(1))] = neu;

	nquad.setVerticalInterconnect(X(0), false);
	nquad.setVerticalInterconnect(X(1), false);
	nquad.setHorizontalInterconnect(Y(0), false);
	nquad.setHorizontalInterconnect(Y(1), false);

	HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

	// FPGA FIFO input
	//HICANN::GbitLink gl;
	gl.dirs[0] = HICANN::GbitLink::Direction::TO_HICANN;
	gl.dirs[7] = HICANN::GbitLink::Direction::TO_HICANN;

	DNC::GbitReticle gr;
	gr[h.to_HICANNOnDNC()] = gl;

	HICANN::set_gbit_link(h, gl);
	//DNC::set_hicann_directions(f.dnc(d), gr);
	DNC::set_hicann_directions(f, d, gr);

	usleep(1000); //wait for syndrivers to be locked then start STDP


		// STDP stuff
		/*HICANN::weight_row_t weights;
		weights = HICANN::get_weights_row(h, Coordinate::SynapseDriver(EnumLocal(111), geometry::left), true);
		cout << "Initial weight of the STDP-synapse is: " << static_cast<int>(weights[0].value()) << endl;

		stdp = STDPControl(); //default values should be OK
		HICANN::set_stdp_config(h, top, stdp);*/
	HICANN::WeightRow weights;
	weights = HICANN::get_weights_row(h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), bottom));
	log(Logger::INFO) << "Initial weight of the STDP-synapse is: " << static_cast<int>(weights[0].value()) << endl;

	STDPControl stdp = STDPControl(); //default values should be OK
	HICANN::set_stdp_config(h, top, stdp);


		//switch on analog output
		/*Analog aout;
		aout.set_preout(Coordinate::Analog(0));
		aout.set_membrane_top_even(Coordinate::Analog(1));
		aout.enable(Coordinate::Analog(0));
		aout.enable(Coordinate::Analog(1));
		HICANN::set_analog(h, aout);

		//turn off Background Generator
		bg.enable(false);
		for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
		HICANN::set_background_generator(h, bgarray);*/
	HICANN::Analog aout;
	aout.set_preout(Coordinate::AnalogOnHICANN(0));
	aout.set_membrane_top_even(Coordinate::AnalogOnHICANN(1));
	aout.enable(Coordinate::AnalogOnHICANN(0));
	aout.enable(Coordinate::AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);

	//turn off Background Generator
	bg.enable(false);
	for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
	HICANN::set_background_generator(h, bgarray);

	}

	void common_finish() {
		//switch off the DNC links
		for (int i = 0; i < 8; i++) gl.dirs[i] = HICANN::GbitLink::Direction::OFF;
		for (auto hod: Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) gr[hod] = gl;
		HICANN::set_gbit_link(h, gl);
		DNC::set_hicann_directions(f, d, gr);
	}
};

/**
 * -configure one neuron with two synapses: one strong and one weak.
 * -the strong one brings neuron over the threshold, the weak one is
 *  for the measurement.
 * -the second synapse fires shortly after the first one to cause the
 *  STDP-mechanism to recognize it as causal spike-dependency
 * -auto-weight-update is on
 * -STDPLUT::LUT is suggesting an addition of +1 on every threshold crossing
 * -weights are programmed to 0 at first and read out again after the
 *  experiment
 * -spike delay: begin with 30 ns
 */
TEST_F(HICANNSTDPTest, STDPHWTest) {
	FPGA::reset(f);
	//HICANN::reset(h);
	HICANN::init(h, false);

	/*
	FGControl fgc = FGControl();

	//FG parameters for neurons just under the threshold (almost spiking)
	for (size_t neuron_number = 0; neuron_number < 128; neuron_number++)
	{
		Coordinate::NeuronOnHICANN nrn{Enum(neuron_number)};
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 450);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 150);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 796);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_bexp, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convi, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convx, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 0);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 100);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbi, 800);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbx, 800);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 100);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 511);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 300);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_spikeamp, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 500);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syni, 350);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 820);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 820);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_synx, 568);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 500); //threshold neuron0
	}

	fgc.setShared(Coordinate::FGBlockOnHICANN(0), HICANN::shared_parameter::I_breset, 1023);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::I_bstim, 1023);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::int_op_bias, 1023);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_bout, 1023);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_br, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_bstdf, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_ccas, 800);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_clra, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_dep, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_dllres, 200);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_dtc, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_fac, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_gmax0, 1000);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_gmax1, 1000);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_gmax2, 1000);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_gmax3, 1000);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_m, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_reset, 450);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_stdf, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_thigh, 0);
	fgc.setShared(Coordinate::FGBlockOnHICANNOnHICANN(0), HICANN::shared_parameter::V_tlow, 0);

	STDPAnalog stdparam = STDPAnalog();
	fgc.load_stdp_parameters(top, stdparam); //load STDP parameters to the floating gates

	//HICANN::set_PLL_frequency(h, 100);
	HICANN::set_fg_config(h, b, HICANN::FGConfig());
	HICANN::set_fg_values(h, fgc.extractBlock(Coordinate::FGBlockOnHICANNOnHICANN(0))); //write 2 times for better accuracy
	HICANN::set_fg_values(h, fgc.extractBlock(Coordinate::FGBlockOnHICANNOnHICANN(0)));
	//HICANN::set_PLL_frequency(h, 150);

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::Merger mer = HICANN::Merger();
	for (size_t  i = 0; i < 8; i++){
		mer.config = HICANN::Merger::MERGE; //syndriver locking comes from HICANN BEG, input - from L2
		mer.slow = true;
		mergers[i] = mer;
		mergers.set_loopback(i, HICANN::Loopback(false)); //disable all loopbacks
	}
	HICANN::set_dnc_merger(h, mergers);

	// configure merger tree, phase
	HICANN::MergerTree tree = HICANN::MergerTree();
	HICANN::phase_t phase = 0;
	HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);

	// configure Background Generator
	HICANN::BackgroundGeneratorArray bgarray;
	HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();

	uint16_t period = 1000;
	bg.enable(true);
	bg.random(false);
	bg.seed(200);
	bg.period(period);
	bg.address(HICANN::L1Address(0));

	//all BEGs send locking packets
	for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
	HICANN::set_background_generator(h, bgarray);

	// Enable HICANN::Repeater
	HICANN::set_repeater_block(h, Coordinate::RepeaterBlockOnHICANN(X(0), Y(1)), HICANN::RepeaterBlock());

	HICANN::HorizontalRepeater sr;
	// enable output on this hicann
	sr.setOutput(right);

	Coordinate::OutputBufferOnHICANN bg7(7), bg0(0);
	Coordinate::HorizontalRepeaterOnHICANN bg7rep = bg7.repeater();
	Coordinate::HorizontalRepeaterOnHICANN bg0rep = bg0.repeater();

	ASSERT_EQ(Coordinate::HLineOnHICANN( 6), bg7rep.horizontal().line());
	ASSERT_EQ(Coordinate::HLineOnHICANN(62), bg0rep.horizontal().line());
	HICANN::set_repeater(h, bg7rep, sr); //BG7, channel 7 -> syndriver 111 (STDP synapse)
	HICANN::set_repeater(h, bg0rep, sr); //BG0, channel 0 -> syndriver 97 (strong non-STDP synapse)

	// Crossbar Switch
	Crossbar cb;
	HICANN::CrossbarRow row_cfg;

	cb.set(Coordinate::VLineOnHICANN(28), bg7rep.line(), true); //BG7, channel 7 -> syndriver 111
	row_cfg = cb.get_row(bg7rep.line(), left);
	HICANN::set_crossbar_switch_row(h, bg7rep.line(), left, row_cfg);

	cb.set(Coordinate::VLineOnHICANN(0), bg0rep.line(), true); //BG0, channel 0 -> syndriver 97
	row_cfg =  cb.get_row(bg0rep.line(), left);
	HICANN::set_crossbar_switch_row(h, bg0rep.line(), left, row_cfg);

	// Syndriver Switch
	SynapseSwitch sw;
	HICANN::SynapseSwitchRow srow_cfg;
	Coordinate::SynapseSwitchRowOnHICANN addr(Coordinate::SynapseSwitchRowOnHICANN(Y(111), left));

	sw.set(Coordinate::VLineOnHICANN(28), addr.line(), true); //BG7, channel 7 -> syndriver 111
	srow_cfg = sw.get_row(addr);
	HICANN::set_syndriver_switch_row(h, addr, srow_cfg);

	Coordinate::SynapseSwitchRowOnHICANN addr1(Coordinate::SynapseSwitchRowOnHICANN(Y(97), left));

	sw.set(Coordinate::VLineOnHICANN(0), addr1.line(), true); //BG0, channel 0 -> syndriver 97
	srow_cfg = sw.get_row(addr1);
	HICANN::set_syndriver_switch_row(h, addr1, srow_cfg);

	// Synapse Driver
	HICANN::SynapseArrayRow sar;
	auto driver = sar.getDriver();
	driver.set_l1();
	sar.setDriver(driver);
	sar.config[HICANN::SynapseArrayRow::top].decoder[HICANN::LineConfig::left] = 0;
	sar.config[HICANN::SynapseArrayRow::top].decoder[HICANN::LineConfig::right] = 0;
	sar.config[HICANN::SynapseArrayRow::top].gmax_div[left] = 1;
	sar.config[HICANN::SynapseArrayRow::top].gmax_div[right] = 1;
	sar.config[HICANN::SynapseArrayRow::top].syn_in[left] = 1;
	sar.config[HICANN::SynapseArrayRow::top].syn_in[right] = 0;
	sar.config[HICANN::SynapseArrayRow::top].selgmax = 0;
	sar.config[HICANN::SynapseArrayRow::bottom].decoder[HICANN::LineConfig::left] = 0;
	sar.config[HICANN::SynapseArrayRow::bottom].decoder[HICANN::LineConfig::right] = 0;
	sar.config[HICANN::SynapseArrayRow::bottom].gmax_div[left] = 1;
	sar.config[HICANN::SynapseArrayRow::bottom].gmax_div[right] = 1;
	sar.config[HICANN::SynapseArrayRow::bottom].syn_in[left] = 1;
	sar.config[HICANN::SynapseArrayRow::bottom].syn_in[right] = 0;
	sar.config[HICANN::SynapseArrayRow::bottom].selgmax = 0;
	// set_synapse_driver includes toggling the drivers DLL
	HICANN::set_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), sar); //BG7, channel 7 -> syndriver 111
	HICANN::set_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Y(97), left), sar); //BG0, channel 0 -> syndriver 97

	// Synapses
	HICANN::DecoderDoubleRow drow;
	HICANN::WeightRow row;
	std::fill(drow[0].begin(), drow[0].end(), SynapseDecoder(0xf)); //initialize with blocking values
	std::fill(drow[1].begin(), drow[1].end(), SynapseDecoder(0xf));
	std::fill(row.begin(), row.end(), SynapseWeight(0x7)); //initialize with lowest values

	//STDP synapse, initial weight 7, decoder 1
	drow[0][0] = 1;
	HICANN::set_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), drow);
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(111), left, top), row); //top line of a driver
	//row[0] = 7; //only one synapse (lower)
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(111), left, bottom), row); //bottom line of a driver

	//non-STDP synapse (2 synapses combined), max. weight, decoder 2
	drow[0][0] = 2;
	drow[1][0] = 2;
	row[0] = 15;
	HICANN::set_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(Y(97), left), drow);
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(97), left, top), row); //top line of a driver
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(97), left, bottom), row); //bottom line of a driver

	// Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(32)); //not to get mixed up
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(true);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu; //only top left neuron fires
	neu.enable_aout(false);
	neu.activate_firing(false);
	nquad[Coordinate::NeuronOnQuad(X(1), Y(0))] = neu;
	nquad[Coordinate::NeuronOnQuad(X(0), Y(1))] = neu;
	nquad[Coordinate::NeuronOnQuad(X(1), Y(1))] = neu;

	nquad.setVerticalInterconnect(X(0), false);
	nquad.setVerticalInterconnect(X(1), false);
	nquad.setHorizontalInterconnect(Y(0), false);
	nquad.setHorizontalInterconnect(Y(1), false);

	HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

	// FPGA FIFO input
	//HICANN::GbitLink gl;
	gl.dirs[0] = HICANN::GbitLink::Direction::TO_HICANN;
	gl.dirs[7] = HICANN::GbitLink::Direction::TO_HICANN;

	DNC::GbitReticle gr;
	gr[0] = gl;

	HICANN::set_gbit_link(h, gl);
	DNC::set_hicann_directions(f.dnc(d), gr);

	usleep(1000); //wait for syndrivers to be locked then start STDP

	// STDP stuff
	HICANN::WeightRow weights;
	weights = HICANN::get_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(111), left, bottom));
	cout << "Initial weight of the STDP-synapse is: " << weights[0].to_ulong() << endl;

	STDPControl stdp = STDPControl(); //default values should be OK
	HICANN::set_stdp_config(h, top, stdp);

	// Generate FIFO data
	Coordinate::DNCGlobal d(Enum(0));
	FPGA::PulseEventContainer sent_data, received_data;
	sent_data.clear();
	received_data.clear();

	//channel 7 is STDP synapse with neuron number 1,
	//channel 0 is non-STDP synapse with neuron number 2
	//one cycle corresponds to 4 ns (250 MHz DNC-clock)
	//delay syndriver-preout->postsyn spike is 255 +/- 20 ns
	//hence delay between non-STDP synapse and STDP-synapse is 225 ns (56 cycles)
	//delay between spike-pairs is 2 us (500 cycles)
	sent_data.insert_spike({
			1,                             //DNC-number (1 for vertical setup)
			static_cast<uint32_t>(h.coordinate().id()), //HICANN-number
			0,                             //channel (0, 7)
			HICANN::L1Address(2),                  //neuron number (2, 1)
			100});                         //release time in cycles

	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 656});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 1100});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 1156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 1600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 1656});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 2100});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 2156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 2600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 2656});

	//switch on analog output
	HICANN::Analog aout;
	aout.set_preout(Coordinate::AnalogOnHICANN(0));
	aout.set_membrane_top_even(Coordinate::AnalogOnHICANN(1));
	aout.enable(Coordinate::AnalogOnHICANN(0));
	aout.enable(Coordinate::AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);

	//turn off Background Generator
	bg.enable(false);
	for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
	HICANN::set_background_generator(h, bgarray);
	*/
	common_init();
	/*
	// Generate FIFO data
	Coordinate::DNC d(EnumGlobal(0));
	FPGAPulseEventContainer sent_data, received_data;
	sent_data.clear();
	received_data.clear();

	//channel 7 is STDP synapse with neuron number 1,
	//channel 0 is non-STDP synapse with neuron number 2
	//one cycle corresponds to 4 ns (250 MHz DNC-clock)
	//delay syndriver-preout->postsyn spike is 255 +/- 20 ns
	//hence delay between non-STDP synapse and STDP-synapse is 225 ns (56 cycles)
	//delay between spike-pairs is 2 us (500 cycles)
	sent_data.insert_spike({
			1,                             //DNC-number (1 for vertical setup)
			static_cast<uint32_t>(h.coordinate().id()), //HICANN-number
			0,                             //channel (0, 7)
			HICANN::L1Address(2),                  //neuron number (2, 1)
			100});                         //release time in cycles

	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 656});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 1100});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 1156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 1600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 1656});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 2100});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 2156});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 0, HICANN::L1Address(2), 2600});
	sent_data.insert_spike({1, static_cast<uint32_t>(h.coordinate().id()), 7, HICANN::L1Address(1), 2656});


	/// Start STDP and send/receive events
	stdp.continuous_autoupdate = 1;
	stdp.read_causal = 1;
	stdp.read_acausal = 1;
	HICANN::start_stdp(h, top, stdp);
	received_data = FPGA::send_and_receive(d, sent_data, false, 1000); //1 ms, no loop

	//switch analog output off
	aout.disable(Coordinate::AnalogOnHICANN(0));
	aout.disable(Coordinate::AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);

	//stop auto-STDP update
	HICANN::stop_stdp(h, top, stdp);

	STDPControl::corr_row corrpattern = HICANN::stdp_read_correlation(h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), true));

	for (int i=0; i < 255; i++)	{
		cout << corrpattern[0][i] << " ";
		corrpattern[0][i] = 1;
	}
	cout << endl;
	for (int i=0; i < 255; i++)	{
		cout << corrpattern[1][i] << " ";
		corrpattern[1][i] = 1;
	}
	cout << endl;

	HICANN::stdp_reset_capacitors(h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), true), corrpattern);

	corrpattern = HICANN::stdp_read_correlation(h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), true));

	for (int i=0; i < 255; i++)	{
		cout << corrpattern[0][i] << " ";
		corrpattern[0][i] = 1;
	}
	cout << endl;
	for (int i=0; i < 255; i++)	{
		cout << corrpattern[1][i] << " ";
		corrpattern[1][i] = 1;
	}
	cout << endl;

	//read out the new synapse weight:
	weights = HICANN::get_weights_row(h, Coordinate::SynapseRowOnHICANN(Y(111), left, bottom));
	cout << "New weight of the STDP-synapse is: " << weights[0].to_ulong() << " " << weights[1].to_ulong() << " " << weights[2].to_ulong() << " " << weights[3].to_ulong() << " " << weights[4].to_ulong() << endl;

	common_finish();
	
	//switch off the DNC links
	for (int i = 0; i < 8; i++) gl.dirs[i] = HICANN::GbitLink::Direction::OFF;
	for (int i = 0; i < 8; i++) gr[i] = gl;
	HICANN::set_gbit_link(h, gl);
	DNC::set_hicann_directions(f.dnc(d), gr);*/
}

/*
 * Performs null reads on synapses. The result should be b = V_tlow > V_thigh.
 * This is used for causal and acausal pattern, so both reads should return 
 * identical results.
 * The test sweeps V_tlow and V_thigh and counts how many synapses in one row
 * deviate from the expected bit (deviations) and how often the second read differs 
 * from the first one (mismatches).
 */
TEST_F(HICANNSTDPTest, DISABLED_CorrNullReadHWTest) {
	common_init();

	static int const num_repeat = 3;
	static int const start_syndrv = 1;
	static int const stop_syndrv = 223;
	static int const num_tested_rows = stop_syndrv - start_syndrv + 1;
	//Coordinate::SynapseDriver coord_row(EnumLocal(51), geometry::left);
	STDPEval null_read{std::bitset<4>(0), std::bitset<4>(0)};
	std::vector<int> mismatches;
	std::vector<int> deviations;
	std::array<int,256*224*2> dev_per_syn;

	const std::array<std::pair<uint16_t,uint16_t>,12-4> test_cmp_voltages{{
		// v_thigh, V_tlow
		std::make_pair( 1023,    0 ),
		std::make_pair( 1023,  256 ),
		std::make_pair( 1023,  512 ),
		//std::make_pair( 1023,  768 ),
		std::make_pair(  768,  256 ),
		//std::make_pair(  768,  512 ),
		std::make_pair(    0, 1023 ),
		std::make_pair(  256, 1023 ),
		std::make_pair(  512, 1023 ),
		//std::make_pair(  768, 1023 ),
		std::make_pair(  256,  768 ),
		//std::make_pair(  512,  768 ),
	}};

	dev_per_syn.fill(0);

	for(int r=0; r<num_repeat; r++) {
		for(auto cmp_v : test_cmp_voltages) {
			log(Logger::INFO) << "setting V_thigh = " << cmp_v.first 
				<< " V_tlow = " << cmp_v.second 
				<< " ...";
			set_cmp_voltages(cmp_v.first, cmp_v.second, fgc, stdparam);
			log << "done";

			bool expected_bit = (stdparam.V_tlow > stdparam.V_thigh);
			log(Logger::INFO) << "Expecting bit = " << expected_bit;

			stdp.eval = null_read;
			HICANN::set_stdp_config(h, top, stdp);

			for(int syndrv=start_syndrv; syndrv<=stop_syndrv; syndrv++) {
				Coordinate::SynapseDriverOnHICANN coord_row(Y(syndrv), ((syndrv % 2 == 0) ^ (syndrv >= 112)) ? right : left);
				bool line = false;
				do {
					STDPControl::corr_row corrpattern = HICANN::stdp_read_correlation(h, SynapseRowOnHICANN(coord_row, SideVertical(line)));
					unsigned int native_row = 223-(coord_row.line()*2) - (line ? 1 : 0); 
					int mismatch_counter = 0;

					deviations.push_back(0);
					for (int i=0; i < 256; i++)	{
						if( i % 32 == 0 )
							cout << '\n' << "> ";
						cout << corrpattern[0][i];

						if( corrpattern[0][i] != expected_bit ) {
							deviations.back()++;
							dev_per_syn[native_row*256 + i]++;
						}
					}
					cout << endl;
					for (int i=0; i < 256; i++)	{
						if( i % 32 == 0 )
							cout << '\n' << "> ";

						//cout << corrpattern[1][i] << " ";
						if( corrpattern[1][i] > corrpattern[0][i] ) {
							cout << "+";
							mismatch_counter++;
						} else if( corrpattern[1][i] < corrpattern[0][i] ) {
							cout << "-";
							mismatch_counter++;
						} else
							cout << " ";
					}
					cout << "\nmismatch count = " << mismatch_counter << endl;
					mismatches.push_back(mismatch_counter);

					// results should be identical for both reads
					EXPECT_EQ(0, mismatch_counter);

					// results should match to V_tlow > V_thigh
					EXPECT_EQ(0, deviations.back());

					line = !line;
				} while( line );
			}
		}
	}

	cout << "mismatches: ";
	for(auto m : mismatches) {
		cout << m << ", ";
	}
	cout << '\n';

	cout << "deviations: ";
	for(auto d : deviations) {
		cout << d << ", ";
	}
	cout << '\n';

	cout << "Number of synapses without deviations: "
		<< std::count(begin(dev_per_syn), end(dev_per_syn), 0) - (256*224*2 - num_tested_rows*256)
		<< "\nNumber of synapses with less than 1% deviations: ";

	int one_percent_limit = static_cast<int>(static_cast<double>(test_cmp_voltages.size() * num_repeat) * 0.01);
	cout << std::count_if(begin(dev_per_syn), end(dev_per_syn), 
			[one_percent_limit](int v) { return v <= one_percent_limit; }) - (256*224*2 - num_tested_rows*256)
		<< "\nNumber of tested synapses: "
		<< num_tested_rows * 256
		<< '\n';

	common_finish();
}

/*
 * If configured with identity as look-up table, the auto update controller
 * should never change weights. This test checks this for one synapse row.
 * */
TEST_F(HICANNSTDPTest, AutoUpdateNopHWTest) {
	common_init();

	SynapseRowOnHICANN coord_row(SynapseDriverOnHICANN(Y(51), left), top);
	//SynapseRowOnHICANN coord_row(SynapseDriverOnHICANN(Y(0), left), top);
	//STDPEval null_read{std::bitset<4>(0), std::bitset<4>(0)};

	//stdp.eval = null_read;
	set_cmp_voltages(1023, 0, fgc, stdparam);

	for(int i=0; i<10; i++) {
		HICANN::WeightRow row_readback; 
		HICANN::WeightRow row; 
		std::fill(row.begin(), row.end(), SynapseWeight(0x7));
		HICANN::set_weights_row(h, coord_row, row);
		row = HICANN::get_weights_row(h, coord_row);
		//usleep(100000);
		
		stdp_cycle(stdp, top, nop, nop, nop);
		//usleep(100000);
		row_readback = HICANN::get_weights_row(h, coord_row);

		for(int j=0; j<4*32; j++) {
			EXPECT_EQ(row[j], row_readback[j])
				<< "STDP controller incorrectly modified weight."
				<< " [repetition=" << i
				<< ", column=" << j
				<< "]";
		}
		//EXPECT_EQ(row, row_readback);

		for(int i=0; i<8; i++) {
			log(Logger::INFO) << "> ";
			for(int j=0; j<32; j++) {
				log << std::hex << static_cast<int>(row[i*32+j].value()) << std::dec;
			}
			log << "   ";
			for(int j=0; j<32; j++) {
				log << std::hex << static_cast<int>(row_readback[i*32+j].value()) << std::dec;
			}
			log << "   ";
			for(int j=0; j<32; j++) {
				if( row_readback[i*32+j] > row[i*32+j] )
					log << '+';
				else if( row_readback[i*32+j] < row[i*32+j] )
					log << '-';
				else
					log << ' ';
			}
		}
		//log << '\n';
	}

	common_finish();
}

/*
 * Using the null_read evaluation pattern, the automatic weight update can be tested.
 * The parameter voltages V_tlow and V_thigh control, whether correlation bits 
 * are set or not. The identity (nop), increment by one and decrement by one 
 * look-up tables are then used for different initial weights over the complete chip.
 * This should check, that the STDPLUT::LUT is correctly configured, the analog evaluation 
 * circuit is functional, and the automatic controller performs the correct algorithm.
 * */
TEST_F(HICANNSTDPTest, AutoUpdateNullReadHWTest) {
	common_init();

	static int const num_repeat = 1;
	static int const num_test_configs = 3*16;
	static int const step_test_configs = 8;
	static int const start_syndrv = 0;
	//static int const stop_syndrv = 223;
	static int const stop_syndrv = 2;
	//Coordinate::SynapseDriver coord_row(EnumLocal(51), geometry::left);
	STDPEval null_read{std::bitset<4>(0), std::bitset<4>(0)};
	//STDPLUT::LUT inv_inc;
	//STDPLUT::LUT inv_dec;
	static bool const write_errlog = true;
	static std::string const errlog_fn("auto_update_null_read_error_log.txt");

	//for(int i=0; i<16; i++) {
		//size_t rindex = bit::reverse(std::bitset<4>(i)).to_ulong();
		//inv_inc[rindex] = bit::reverse(std::bitset<4>(std::min(i+1, 15)));
		//inv_dec[rindex] = bit::reverse(std::bitset<4>(std::max(i-1, 0)));
	//}
	
	std::ofstream errlog;
	unsigned int err_count = 0;

	struct {
		uint16_t vth;
		uint16_t vtl;
		int modifier;
		STDPLUT::LUT causal;
		STDPLUT::LUT acausal;
		STDPLUT::LUT combined;
		uint8_t iweight;
	} test_configs[num_test_configs] {
		{    0, 1023,  0, lowest, highest, nop, 0x0 },
		{    0, 1023,  0, lowest, highest, nop, 0x1 },
		{    0, 1023,  0, lowest, highest, nop, 0x2 },
		{    0, 1023,  0, lowest, highest, nop, 0x3 },
		{    0, 1023,  0, lowest, highest, nop, 0x4 },
		{    0, 1023,  0, lowest, highest, nop, 0x5 },
		{    0, 1023,  0, lowest, highest, nop, 0x6 },
		{    0, 1023,  0, lowest, highest, nop, 0x7 },
		{    0, 1023,  0, lowest, highest, nop, 0x8 },
		{    0, 1023,  0, lowest, highest, nop, 0x9 },
		{    0, 1023,  0, lowest, highest, nop, 0xa },
		{    0, 1023,  0, lowest, highest, nop, 0xb },
		{    0, 1023,  0, lowest, highest, nop, 0xc },
		{    0, 1023,  0, lowest, highest, nop, 0xd },
		{    0, 1023,  0, lowest, highest, nop, 0xe },
		{    0, 1023,  0, lowest, highest, nop, 0xf },
		{    0, 1023,  1, lowest, highest, inc, 0x0 },
		{    0, 1023,  1, lowest, highest, inc, 0x1 },
		{    0, 1023,  1, lowest, highest, inc, 0x2 },
		{    0, 1023,  1, lowest, highest, inc, 0x3 },
		{    0, 1023,  1, lowest, highest, inc, 0x4 },
		{    0, 1023,  1, lowest, highest, inc, 0x5 },
		{    0, 1023,  1, lowest, highest, inc, 0x6 },
		{    0, 1023,  1, lowest, highest, inc, 0x7 },
		{    0, 1023,  1, lowest, highest, inc, 0x8 },
		{    0, 1023,  1, lowest, highest, inc, 0x9 },
		{    0, 1023,  1, lowest, highest, inc, 0xa },
		{    0, 1023,  1, lowest, highest, inc, 0xb },
		{    0, 1023,  1, lowest, highest, inc, 0xc },
		{    0, 1023,  1, lowest, highest, inc, 0xd },
		{    0, 1023,  1, lowest, highest, inc, 0xe },
		{    0, 1023,  0, lowest, highest, inc, 0xf },
		{    0, 1023,  0, lowest, highest, dec, 0x0 },
		{    0, 1023, -1, lowest, highest, dec, 0x1 },
		{    0, 1023, -1, lowest, highest, dec, 0x2 },
		{    0, 1023, -1, lowest, highest, dec, 0x3 },
		{    0, 1023, -1, lowest, highest, dec, 0x4 },
		{    0, 1023, -1, lowest, highest, dec, 0x5 },
		{    0, 1023, -1, lowest, highest, dec, 0x6 },
		{    0, 1023, -1, lowest, highest, dec, 0x7 },
		{    0, 1023, -1, lowest, highest, dec, 0x8 },
		{    0, 1023, -1, lowest, highest, dec, 0x9 },
		{    0, 1023, -1, lowest, highest, dec, 0xa },
		{    0, 1023, -1, lowest, highest, dec, 0xb },
		{    0, 1023, -1, lowest, highest, dec, 0xc },
		{    0, 1023, -1, lowest, highest, dec, 0xd },
		{    0, 1023, -1, lowest, highest, dec, 0xe },
		{    0, 1023, -1, lowest, highest, dec, 0xf },
	};

	stdp.eval = null_read;

	if( write_errlog ) {
		errlog.open(errlog_fn.c_str());
		errlog 
			<< "# num_test_configs(" << num_test_configs/step_test_configs << ")"
			<< " num_repeat(" << num_repeat << ")"
			<< " syndrv(" << start_syndrv << ".." << stop_syndrv << ")"
			<< " column(256) expected != actual\n";
	}

	for(int j=0; j<num_test_configs; j += step_test_configs) {
		if( (j != 0) && ((test_configs[j].vth != test_configs[j-1].vth) || (test_configs[j].vtl != test_configs[j-1].vtl)) ) {
		    cout << "setting compare voltages vth=" << test_configs[j].vth
			    << " vtl=" << test_configs[j].vtl
			    << "..." << std::flush;
		    set_cmp_voltages(test_configs[j].vth, test_configs[j].vtl, fgc, stdparam);  // correlation bit always set
		    cout << "done" << endl;
		}

		for(int i=0; i<num_repeat; i++) {
			for(int syndrv=start_syndrv; syndrv<=stop_syndrv; syndrv++) {
				bool line = false;
				do {
					SynapseRowOnHICANN coord_row(SynapseDriverOnHICANN(Y(syndrv),
							((syndrv % 2 == 0) ^ (syndrv >= 112)) ? geometry::right : geometry::left),
							line ? top : bottom);
					HICANN::WeightRow row_check, row_readback; 
					HICANN::WeightRow row; 
					std::fill(row.begin(), row.end(), SynapseWeight(test_configs[j].iweight));
					HICANN::set_weights_row(h, coord_row, row);
					row_check = HICANN::get_weights_row(h, coord_row);
					ASSERT_EQ(row, row_check) << "Failed to write row of synapse weights.";
					//usleep(100000);
					
					stdp_cycle(stdp, (syndrv >= 112) ? bottom : top, test_configs[j].causal, test_configs[j].acausal, test_configs[j].combined);
					HICANN::flush(h);
					usleep(100000);
					row_readback = HICANN::get_weights_row(h, coord_row);
					
					auto expected = std::max(std::min(row[j]+test_configs[j].modifier, 0xF), 0);
					for(int k=0; k<8*32; k++) {
						bool unmodified = (row_readback[k] == row[k]);
						EXPECT_EQ(expected, static_cast<int>(row_readback[k]))
							<< "Reading incorrect weight from synapses after STDP controller was active "
							<< (unmodified ? "(unmodified)" : "(modified incorrectly)")
							<< " [test config=" << j
							<< ", repetition=" << i
							<< ", syndrv=" << syndrv
							<< ", column=" << k
							<< "]"; 

						if( expected != static_cast<int>(row_readback[k]) ) {
							++err_count;
							if( write_errlog ) {
								errlog 
									<< j
									<< " " << i
									<< " " << syndrv
									<< " " << k
									<< " " << expected
									<< " != " << static_cast<int>(row_readback[k])
									<< "\n";
							}
						}
					}
					//EXPECT_EQ(row_check, row_readback);

					if( write_errlog )
						errlog.flush();

					cout << "\n" << syndrv << ":\n";
					for(int i=0; i<8; i++) {
						cout << "\n> ";
						for(int j=0; j<32; j++) {
							cout << std::hex << static_cast<unsigned int>(row_check[i*32+j].value()) << std::dec;
						}
						cout << "   ";
						for(int j=0; j<32; j++) {
							cout << std::hex << static_cast<unsigned int>(row_readback[i*32+j].value()) << std::dec;
						}
						cout << "   ";
						for(int j=0; j<32; j++) {
							if( row_readback[i*32+j] > row_check[i*32+j] )
								cout << '+';
							else if( row_readback[i*32+j] < row_check[i*32+j] )
								cout << '-';
							else
								cout << ' ';
						}
					}
					cout << '\n';

					line = !line;
				} while( line );
			}
		}
	}

	if( write_errlog )
		errlog.close();

	common_finish();

	cout << "Error count: " << err_count << endl;
}

TEST_F(HICANNSTDPTest, DISABLED_CorrEvalCharacterizationHWTest) {
	//common_init();

	static int const num_repeat = 10;
	static int const start_syndrv = 1;
	static int const stop_syndrv = 111;
	static int const center_voltage = 750;  // 512;
	static int const max_delta = 18; //18;  // positive delta means vth > vtl
	static int const min_delta = -18; //-80;
	static int const delta_step = 1;//1;
	STDPEval null_read{std::bitset<4>(0), std::bitset<4>(0)};
	std::vector<std::pair<uint16_t,uint16_t> > voltages;
	std::vector<std::array<int,num_repeat> > counts;
	std::vector<std::array<std::array<int,256>,num_repeat> > counts_per_col;
	std::ofstream datalog("eval_char_datalog.csv");

	datalog << "# center_voltage, delta, repetition, synapse driver, line, {bit 0, bit 1, sum}*256\n";

	for(int delta=max_delta; delta>=min_delta; delta -= delta_step) {
		uint16_t vth = static_cast<uint16_t>(center_voltage + delta);
		uint16_t vtl = static_cast<uint16_t>(center_voltage - delta);

		minimal_init(vth, vtl);

		voltages.push_back(make_pair(vth, vtl));
		counts.push_back(std::array<int,num_repeat>());
		counts_per_col.push_back(std::array<std::array<int,256>, num_repeat>());
		//log(Logger::INFO) << "counts: " << flush;
		for(int r=0; r<num_repeat; r++) {
			log(Logger::INFO) << "setting V_thigh = " << vth
				<< " V_tlow = " << vtl
				<< " ..." << std::flush;
			//set_cmp_voltages(vth, vtl, fgc, stdparam);
			log << "done";

			counts.back()[r] = 0;
			for(auto& v : counts_per_col.back())
				v.fill(0);

			stdp.eval = null_read;
			HICANN::set_stdp_config(h, top, stdp);

			for(int syndrv=start_syndrv; syndrv<=stop_syndrv; syndrv++) {
				bool line = false;
				do {
					Coordinate::SynapseDriverOnHICANN coord_row(Y(syndrv), 
							((syndrv % 2 == 0) ^ (syndrv >= 112)) ? geometry::right : geometry::left);
					STDPControl::corr_row corrpattern = HICANN::stdp_read_correlation(h, SynapseRowOnHICANN(coord_row, SideVertical(line)));

					counts.back()[r] += std::count_if(begin(corrpattern[0]), end(corrpattern[0]), [](bool b){ return b; });
					counts.back()[r] += std::count_if(begin(corrpattern[1]), end(corrpattern[1]), [](bool b){ return b; });

					datalog << center_voltage << ", " << delta << ", " << r << ", " << syndrv << ", "
						<< (line ? "1" : "0");

					for (int i=0; i < 256; i++)	{
						counts_per_col.back()[r][i] += corrpattern[0][i] + corrpattern[1][i];
						//if( i % 32 == 0 )
							//log(Logger::INFO) << '\n' << "> ";
						//log(Logger::INFO) << corrpattern[0][i];
						datalog << ", " << corrpattern[0][i] << ", " << corrpattern[1][i] << ", "
							<< corrpattern[0][i] + corrpattern[1][i];
					}
					datalog << '\n';
					//cout << endl;

					line = !line;
				} while( line );
			}
			datalog << std::flush;

			log(Logger::INFO) << "Vth = " << vth << " Vtl = " << vtl << " repeat = " << r
				<< counts.back()[r];
		}
		//cout << '\n';

		HICANN::flush(h);
	}

	log(Logger::INFO) << "Bit counts (writing to 'eval_char.txt'):";
	{
		std::ofstream fout("eval_char.txt");

		auto v_iter = begin(voltages);
		for(auto count_iter : counts) {
			uint16_t vth = v_iter->first;
			uint16_t vtl = v_iter->second;
			
			log << "(" << std::setw(4) << vth << ", " << std::setw(4) << vtl << ")    :  ";
			double mean_count = 0.0;
			double var_count = 0.0;
			for(auto count : count_iter) {
				log << count << ", ";
				mean_count += static_cast<double>(count);
			}
			mean_count /= static_cast<double>(num_repeat);
			for(auto count : count_iter) {
				var_count += (static_cast<double>(count) - mean_count)*(static_cast<double>(count) - mean_count);
			}
			var_count = std::sqrt(var_count / static_cast<double>(count_iter.size() - 1));

			fout << vth << ", " << vtl << ", " << mean_count << ", " << var_count << '\n'; 

			++v_iter;
		}
		log(Logger::INFO) << "-----" << endl;
	}

	{
		std::ofstream fout("eval_char_per_col.txt");

		auto v_iter = begin(voltages);
		for(auto testcase : counts_per_col) {
			uint16_t vth = v_iter->first;
			uint16_t vtl = v_iter->second;

			std::array<double,256> mean_count;
			std::array<double,256> var_count;

			mean_count.fill(0.0);
			var_count.fill(0.0);
			for(auto repetition : testcase) {
				for(int i=0; i<256; i++) {
					mean_count[i] += static_cast<double>(repetition[i]);
				}
			}
			for(int i=0; i<256; i++) {
				mean_count[i] /= static_cast<double>(testcase.size());
			}

			for(auto repetition : testcase) {
				for(int i=0; i<256; i++) {
					var_count[i] += (static_cast<double>(repetition[i]) - mean_count[i])
						           *(static_cast<double>(repetition[i]) - mean_count[i]);
				}
			}
			for(int i=0; i<256; i++) {
				var_count[i] = std::sqrt(var_count[i] / static_cast<double>(num_repeat -1));
			}

			fout << vth << ", " << vtl;
			for(int i=0; i<256; i++) {
				fout << ", " << mean_count[i] << ", " << var_count[i];
			}
			fout << '\n';

			++v_iter;
		}
	}

	common_finish();
}

TEST_F(HICANNSTDPTest, ActivateSTDPHWTest) {
	common_init(); 

	//cout << "set_cmp_voltages" << endl;
	//set_cmp_voltages(0, 1023, fgc, stdparam);

	stdp.continuous_autoupdate = 1;
	stdp.set_first_row(0);
	stdp.set_last_row(223);
	stdp.lut.causal = nop;
	stdp.lut.acausal = nop;
	stdp.lut.combined = nop;

	cout << "starting STDP" << endl;
	HICANN::set_stdp_config(h, top, stdp);
	HICANN::start_stdp(h, top, stdp);
	//usleep(100000);  // does not work without this usleep
	//HICANN::wait_stdp(h, side, stdp);
	//HICANN::stop_stdp(h, side, stdp);

	/*cout << "type something to quit" << endl;
	string a;
	cin >> a;*/
	

	if( false ) {
		// configure Background Generator
		HICANN::BackgroundGeneratorArray bgarray;
		HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();

		uint16_t period = 1000;
		bg.enable(true);
		bg.random(false);
		bg.seed(200);
		bg.period(period);
		bg.address(HICANN::L1Address(0));

		//all BEGs send locking packets
		for (size_t i = 0; i < 8; i++) bgarray[i] = bg;
		HICANN::set_background_generator(h, bgarray);
	}

	HICANN::flush(h);

	//cin >> a;

	common_finish();
}

TEST_F(HICANNSTDPTest, DeactivateSTDPHWTest) {
	common_init();

	stdp.continuous_autoupdate = 1;
	stdp.set_first_row(0);
	stdp.set_last_row(223);
	stdp.lut.causal = nop;
	stdp.lut.acausal = nop;
	stdp.lut.combined = nop;

	HICANN::set_stdp_config(h, top, stdp);
	//HICANN::start_stdp(h, side, stdp);
	//usleep(100000);  // does not work without this usleep
	//HICANN::wait_stdp(h, side, stdp);
	HICANN::stop_stdp(h, top, stdp);

	common_finish();
}
#undef RET
#undef HCL1
#undef HCFG
#undef HCREP
#undef HCSYN

} // namespace HMF
