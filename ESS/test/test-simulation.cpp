//implements very simple "networks" on the ESS.
//these are not actually test, because there are no test-conditions
//disabled by default

#include "map"
#include "ess-test-util.h"

#include "hal/HICANN.h"
#include "ESS/halbe_to_ess.h"

#include "hal/backend/HICANNBackend.h"
#include "hal/backend/DNCBackend.h"
#include "hal/backend/FPGABackend.h"

namespace HMF {
using namespace geometry;

TEST_F(ESSTest, DISABLED_BGTest)
{
	HICANN::reset(h);

    HICANN::FGControl fgc;

    load_pattern_naud(fgc,0,"custom");

    //set params for STP
    for(size_t ii = 0; ii < 4; ++ii)
    {
        Coordinate::FGBlockOnHICANN shrd{Enum{ii}};
        fgc.setShared(shrd, HICANN::shared_parameter::V_stdf, 511);
        fgc.setShared(shrd, HICANN::shared_parameter::V_fac, 511);
        fgc.setShared(shrd, HICANN::shared_parameter::V_dtc, 511);
    }

	//set the parameters
	HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::FGConfig());
	HICANN::set_fg_values(h, Coordinate::FGBlockOnHICANN(Enum(0)), fgc[Coordinate::FGBlockOnHICANN(Enum(0))]);

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++)
    {
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

    // configure merger tree, phase
    HICANN::MergerTree tree = HICANN::MergerTree(); //default settings are OK
    HICANN::Phase phase = 0;
    HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);

	// configure Background Generator
	HICANN::BackgroundGeneratorArray bgarray;
	HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();
	for (size_t i = 0; i < 8; i++)
		bgarray[i] = bg;

	uint16_t period = 10000;

	bg.enable(true);
	bg.random(true);
	bg.seed(200);
	bg.period(period);
	bg.address(HICANN::L1Address(0));
	bgarray[7] = bg;
	HICANN::set_background_generator(h, bgarray);

	HICANN::HorizontalRepeater sr;
	// enable output on this hicann
	sr.setOutput(geometry::right);

	Coordinate::SendingRepeaterOnHICANN bg7(0);
	Coordinate::SendingRepeaterOnHICANN bg0(7);
	ASSERT_EQ(6 , bg7.toHLineOnHICANN());
	ASSERT_EQ(62, bg0.toHLineOnHICANN());

	HICANN::set_repeater(h, bg7.toHRepeaterOnHICANN(), sr); //BG -> neuron0
	HICANN::set_repeater(h, bg0.toHRepeaterOnHICANN(), sr); //neuron0 -> neuron1

	// Crossbar Switch
	HICANN::Crossbar cb;
	HICANN::CrossbarRow row_cfg;

	cb.set(Coordinate::VLineOnHICANN(28), bg7.toHLineOnHICANN(), true); //BG 7(?)-> neuron0
	row_cfg = cb.get_row(bg7.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg7.toHLineOnHICANN(), left, row_cfg);

	cb.set(Coordinate::VLineOnHICANN(0), bg0.toHLineOnHICANN(), true); //neuron0 -> neuron1
	row_cfg =  cb.get_row(bg0.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg0.toHLineOnHICANN(), left, row_cfg);
	
	// Syndriver Switch
	HICANN::SynapseSwitch sw;
	HICANN::SynapseSwitchRow srow_cfg;
	Coordinate::SynapseSwitchRowOnHICANN addr(Coordinate::SynapseSwitchRowOnHICANN(Y(111), left));

	sw.set(Coordinate::VLineOnHICANN(28), addr.line(), true); //BG 7(?) -> neuron0
	srow_cfg = sw.get_row(addr);
	HICANN::set_syndriver_switch_row(h, addr, srow_cfg);

	// Synapse Driver
	HICANN::SynapseDriver driver;
	driver.set_l1();
	driver[top].set_decoder(top, HICANN::DriverDecoder(0));
	driver[top].set_decoder(bottom, HICANN::DriverDecoder(0));
	driver[top].set_gmax_div(left, 1);
	driver[top].set_gmax_div(right, 1);
	driver[top].set_syn_in(left, 1);
	driver[top].set_syn_in(right, 0);
	driver[top].set_gmax(0);
	driver[bottom] = driver[top];
	
    // set_synapse_driver includes toggling the drivers DLL
	HICANN::set_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), driver); //BG -> neuron0
	
    // Synapses
	HICANN::DecoderDoubleRow drow;
	HICANN::WeightRow row;
	std::fill(drow[0].begin(), drow[0].end(), HICANN::SynapseDecoder(0xf)); //initialize with blocking values
	std::fill(drow[1].begin(), drow[1].end(), HICANN::SynapseDecoder(0xf));
	std::fill(row.begin(), row.end(), HICANN::SynapseWeight(0x0)); //initialize with lowest values

	//BG -> neuron0
	drow[0][0] = HICANN::SynapseDecoder(0); drow[1][0] = HICANN::SynapseDecoder(0); //set left decoders to 0
	row[0] = HICANN::SynapseWeight(1); //set left weights to max, right weights to 0
	HICANN::set_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), drow);
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Coordinate::SynapseDriverOnHICANN(Y(111), left), top), row); //top line of a driver
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Coordinate::SynapseDriverOnHICANN(Y(111), left), bottom), row); //bottom line of a driver
	
    // Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(false);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu;
	
    HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

	//run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	//disable Test Mode
	fpga.runESS(5*1e4);	
}


TEST_F(ESSTest, DISABLED_OneNeuronTest)
{
	HICANN::reset(h);

    HICANN::FGControl fgc;

    size_t neuron = 0;

    //possible pattern: custom (only naud), tonic, adaptation, initial_burst, tonic_burst, phasic (only binh), chaos (only binh) <- cos binh is chaosbinh
    //load_pattern_naud(fgc, neuron, "tonic_burst");
    load_pattern_naud(fgc, neuron, "custom");

	//set the parameters
	HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::FGConfig());
	HICANN::set_fg_values(h, Coordinate::FGBlockOnHICANN(Enum(0)), fgc[Coordinate::FGBlockOnHICANN(Enum(0))]);

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++)
    {
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

    // configure merger tree, phase
    HICANN::MergerTree tree = HICANN::MergerTree(); //default settings are OK
    HICANN::Phase phase = 0;
    HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);

    //current stimulus
    //Naud: tonic,adaptation: 205; initial_burst: 164; regular_burst: 86
    //tonic: 300; adaptation: 200; initial_burst: ??; regular_burst: ??; phasic: 205
    //HICANN::FGStimulus s;
    //s.setPulselength(15);
    //s.setContinuous(true);
    //for(size_t i = 0; i < s.size(); ++i)
    //{
    //    if(i < 10)
    //        s[i] = 0;
    //    else
    //        s[i] = 210;
    //}
    //HICANN::set_current_stimulus(h, Coordinate::FGBlockOnHICANN(Coordinate::Enum(0)), s);
    
    // Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);


	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(false);
    neu.enable_current_input(true);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu;
	
    HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

    //run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	//disable Test Mode
   	fpga.runESS(2*1e5);		//i dunno how many ns...
}


TEST_F(ESSTest, DISABLED_StatisticTest)
{
	HICANN::reset(h);
    HICANN::FGControl fgc;
    for(size_t neuron = 0; neuron < 512; neuron++)
    {
        Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{neuron}};
        Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN(); 
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 900);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 600);  
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 600);  
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 22);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 550);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 1023);   
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 80);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 800);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 400);     
        fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 200);
    }
    for(size_t i = 0; i < 4; ++i)
    {
        Coordinate::FGBlockOnHICANN fgb{Coordinate::Enum{i}};
		HICANN::set_fg_config(h, fgb, HICANN::FGConfig());
        HICANN::set_fg_values(h, fgb, fgc[fgb]);
    }

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++)
    {
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

    // configure merger tree, phase
    HICANN::MergerTree tree = HICANN::MergerTree(); //default settings are OK
    HICANN::Phase phase = 0;
    HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);
    
    // Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
    nconf.bigcap[geometry::bottom] = true;
    nconf.bigcap[geometry::top] = true;
    HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(false);
    neu.enable_current_input(false);

    for(size_t quadpos = 0; quadpos < 4; quadpos++)
    {
        Coordinate::NeuronOnQuad nrnqu{Coordinate::Enum{quadpos}};
        nquad[nrnqu] = neu;
    }

    for(size_t quad = 0; quad < 128; quad++)
    {
        Coordinate::QuadOnHICANN qu(quad);
        HICANN::set_denmem_quad(h, qu, nquad);
    }
	
    //run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	//disable Test Mode
	fpga.runESS(1e4);
}

TEST_F(ESSTest, DISABLED_TwoNeuronTest)
{
	using namespace Coordinate;
	HICANN::init(this->h, false);

    HICANN::FGControl fgc;
	//FG parameters for neurons just under the threshold (almost spiking)
	for (size_t neuron_number = 0; neuron_number < 2; neuron_number++)
	{
		Coordinate::NeuronOnHICANN nrn{Enum(neuron_number)};
		Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN();

		//test params for Sebastian...
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 169);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 227);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 691);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 23);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 348);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 34);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 700);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 973);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 195);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);
        fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 452);
        fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 169);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 450);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 650);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 650);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 22);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 1000);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 10);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 1023);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 300);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 700);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 790);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 790);
		//fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 455); //threshold neuron0
    }
	
	typedef FGBlockOnHICANN fgb;
	HICANN::set_fg_config(this->h, fgb(Enum(0)), HICANN::FGConfig());
	HICANN::set_fg_config(this->h, fgb(Enum(1)), HICANN::FGConfig());
	HICANN::set_fg_values(this->h, fgb(Enum(0)), fgc.getBlock(fgb(Enum(0))));
	HICANN::set_fg_values(this->h, fgb(Enum(1)), fgc.getBlock(fgb(Enum(1))));

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++){
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(this->h, mergers);

	// configure merger tree, phase
	HICANN::MergerTree tree; //default settings are OK
	HICANN::Phase phase = 0;
	HICANN::set_merger_tree(this->h, tree);
	HICANN::set_phase(this->h, phase);

	// configure Background Generator
	HICANN::BackgroundGeneratorArray bgarray;
	HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();
	for (size_t i = 0; i < 8; i++) bgarray[i] = bg;

	uint16_t period = 10000;

	bg.enable(true);
	bg.random(false);
	bg.seed(200);
	bg.period(period);
	bg.address(HICANN::L1Address(0));
	bgarray[7] = bg;
	HICANN::set_background_generator(this->h, bgarray);

	// Enable HICANN::Repeater
	HICANN::set_repeater_block(this->h, RepeaterBlockOnHICANN(X(0), Y(1)), HICANN::RepeaterBlock());

	HICANN::HorizontalRepeater sr;
	// enable output on this hicann
	sr.setOutput(right);

	SendingRepeaterOnHICANN bg7(0);
	SendingRepeaterOnHICANN bg0(7);
	ASSERT_EQ(62, bg0.toHLineOnHICANN());
	ASSERT_EQ( 6, bg7.toHLineOnHICANN());

	HICANN::set_repeater(this->h, bg7.toHRepeaterOnHICANN(), sr); //BG -> neuron0
	HICANN::set_repeater(this->h, bg0.toHRepeaterOnHICANN(), sr); //neuron0 -> neuron1

	// Crossbar Switch
	HICANN::Crossbar cb;
	HICANN::CrossbarRow row_cfg;

	cb.set(VLineOnHICANN(28), bg7.toHLineOnHICANN(), true); //BG -> neuron0
	row_cfg = cb.get_row(bg7.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(this->h, bg7.toHLineOnHICANN(), left, row_cfg);

	cb.set(VLineOnHICANN(0), bg0.toHLineOnHICANN(), true); //neuron0 -> neuron1
	row_cfg =  cb.get_row(bg0.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(this->h, bg0.toHLineOnHICANN(), left, row_cfg);

	// Syndriver Switch
	HICANN::SynapseSwitch sw;
	HICANN::SynapseSwitchRow srow_cfg;
	SynapseSwitchRowOnHICANN addr(SynapseSwitchRowOnHICANN(Y(111), left));

	sw.set(VLineOnHICANN(28), addr.line(), true); //BG -> neuron0
	srow_cfg = sw.get_row(addr);
	HICANN::set_syndriver_switch_row(this->h, addr, srow_cfg);

	SynapseSwitchRowOnHICANN addr1(SynapseSwitchRowOnHICANN(Y(97), left));
	sw.set(VLineOnHICANN(0), addr1.line(), true); //neuron0 -> neuron1
	srow_cfg = sw.get_row(addr1);
	HICANN::set_syndriver_switch_row(this->h, addr1, srow_cfg);

	// Synapse Driver
	HICANN::SynapseDriver driver;
	driver.set_l1();
	driver[top].set_decoder(top, HICANN::DriverDecoder(0));
	driver[top].set_decoder(bottom, HICANN::DriverDecoder(0));
	driver[top].set_gmax_div(left, 1);
	driver[top].set_gmax_div(right, 1);
	driver[top].set_syn_in(left, 1);
	driver[top].set_syn_in(right, 0);
	driver[top].set_gmax(0);
	driver[bottom] = driver[top];

	// set_synapse_driver includes toggling the drivers DLL
	HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Y(111), left), driver); //BG -> neuron0
	HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Y(97), left), driver); //neuron0 -> neuron1

	// Synapses
	HICANN::DecoderDoubleRow drow;
	HICANN::WeightRow row;
	std::fill(drow[0].begin(), drow[0].end(), HICANN::SynapseDecoder(0xf)); //initialize with blocking values
	std::fill(drow[1].begin(), drow[1].end(), HICANN::SynapseDecoder(0xf));
	std::fill(row.begin(), row.end(), HICANN::SynapseWeight(0x0)); //initialize with lowest values

	//BG -> neuron0
	drow[0][0] = HICANN::SynapseDecoder(0); drow[1][0] = HICANN::SynapseDecoder(0); //set left decoders to 0
	row[0] = HICANN::SynapseWeight(10); //set left weights to max, right weights to 0
	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(111), left), drow);
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), top), row); //top line of a driver
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(111), left), bottom), row); //bottom line of a driver

	//neuron0 -> neuron1
	// set left decoders to 15, right decoders to 0
	drow[0][0] = HICANN::SynapseDecoder(15); drow[1][0] = HICANN::SynapseDecoder(15);
	drow[0][1] = HICANN::SynapseDecoder(0); drow[1][1] = HICANN::SynapseDecoder(0);
	row[0] = HICANN::SynapseWeight(0); row[1] = HICANN::SynapseWeight(10); //set right weights to max, left weights to 0
	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(97), left), drow);
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(97), left), top), row); //top line of a driver
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(97), left), bottom), row); //bottom line of a driver

	// Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(this->h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_spl1_output(true);
	neu.enable_fire_input(false);
	neu.enable_aout(true);

	nquad[NeuronOnQuad(X(0), Y(0))] = neu; //only two neurons fire
	neu.address(HICANN::L1Address(32)); //not to get mixed up
	nquad[NeuronOnQuad(X(1), Y(0))] = neu;
	neu.enable_aout(false);
	neu.activate_firing(false);
	neu.enable_spl1_output(false);
	nquad[NeuronOnQuad(X(0), Y(1))] = neu;
	nquad[NeuronOnQuad(X(1), Y(1))] = neu;

	nquad.setVerticalInterconnect(X(0), false);
	nquad.setVerticalInterconnect(X(1), false);
	nquad.setHorizontalInterconnect(Y(0), false);
	nquad.setHorizontalInterconnect(Y(1), false);

	HICANN::set_denmem_quad(this->h, QuadOnHICANN(0), nquad);

	//run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	//disable Test Mode
    fpga.runESS(5*1e5);
}

//TODO Merge with Input Test
TEST_F(ESSTest, DISABLED_FPGAReadoutTest)
{
	HICANN::reset(h);

    HICANN::FGControl fgc;

    size_t neuron = 0;

    //possible pattern: custom (only naud), tonic, adaptation, initial_burst, tonic_burst, phasic (only binh), chaos (only binh) <- cos binh is chaosbinh
    //load_pattern_naud(fgc, neuron, "tonic_burst");
    // load_pattern_naud(fgc, neuron, "custom");
    load_pattern_naud(fgc, neuron, "tonic");

	Coordinate::NeuronOnHICANN nrn{Enum(neuron)};
    Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN();
	// // Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN();
	fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 1000);
	fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 1000);
	fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 0);

	//set the parameters
	HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::FGConfig());
	HICANN::set_fg_values(h, Coordinate::FGBlockOnHICANN(Enum(0)), fgc[Coordinate::FGBlockOnHICANN(Enum(0))]);

	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++)
    {
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

    // configure merger tree, phase
    HICANN::MergerTree tree = HICANN::MergerTree(); //default settings are OK
    HICANN::Phase phase = 0;
    HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);

	//configure the GBitReticle
	HICANN::GbitLink link;
	for (size_t i = 0; i < 8; i++)
	{
		link[Coordinate::GbitLinkOnHICANN(i)] = HICANN::GbitLink::Direction::TO_DNC;
	}	
	HICANN::set_gbit_link(h,link);

    // Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(false);
    neu.enable_current_input(true);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu;
	
    HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

	//congiure the FPGA
	FPGA::prime_experiment(fpga);
	FPGA::start_experiment(fpga);

    //run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	//disable Test Mode
   	fpga.runESS(1e5);		//i dunno how many ns...
	//readout Pulses
	auto const pulses = FPGA::read_trace_pulses(fpga, 0 /* runtime not needed */).events;

	std::cout << "Number of pulses: " << pulses.size() << std::endl;

	ASSERT_EQ( 39, pulses.size());

	for(size_t ii = 0; ii < pulses.size(); ii++)
	{
		std::cout << pulses[ii] << std::endl;
	}
}

//TODO Merge with Readout Test
TEST_F(ESSTest, DISABLED_FPGAInputTest)
{
	HICANN::reset(h);

    HICANN::FGControl fgc;

    load_pattern_naud(fgc,0,"custom");

    //set params for STP
    for(size_t ii = 0; ii < 4; ++ii)
    {
        Coordinate::FGBlockOnHICANN shrd{Enum{ii}};
        fgc.setShared(shrd, HICANN::shared_parameter::V_stdf, 511);
        fgc.setShared(shrd, HICANN::shared_parameter::V_fac, 511);
        fgc.setShared(shrd, HICANN::shared_parameter::V_dtc, 511);
    }

	//set the parameters
	HICANN::set_fg_config(h, Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::FGConfig());
	HICANN::set_fg_values(h, Coordinate::FGBlockOnHICANN(Enum(0)), fgc[Coordinate::FGBlockOnHICANN(Enum(0))]);

	//configure the GBitReticle
	HICANN::GbitLink link;
	for (size_t i = 0; i < 8; i++)
	{
		link[Coordinate::GbitLinkOnHICANN(i)] = HICANN::GbitLink::Direction::TO_HICANN;
	}	
	HICANN::set_gbit_link(h,link);
	
	// configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for (size_t  i = 0; i < 8; i++)
    {
		mer.config = HICANN::Merger::LEFT_ONLY;
		mer.slow = true;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(i)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

    // configure merger tree, phase
    HICANN::MergerTree tree = HICANN::MergerTree(); //default settings are OK
    HICANN::Phase phase = 0;
    HICANN::set_merger_tree(h, tree);
	HICANN::set_phase(h, phase);

	HICANN::HorizontalRepeater sr;
	// enable output on this hicann
	sr.setOutput(geometry::right);

	Coordinate::SendingRepeaterOnHICANN bg7(0);
	Coordinate::SendingRepeaterOnHICANN bg0(7);
	ASSERT_EQ(6 , bg7.toHLineOnHICANN());
	ASSERT_EQ(62, bg0.toHLineOnHICANN());

	HICANN::set_repeater(h, bg7.toHRepeaterOnHICANN(), sr); //BG -> neuron0
	HICANN::set_repeater(h, bg0.toHRepeaterOnHICANN(), sr); //neuron0 -> neuron1

	// Crossbar Switch
	HICANN::Crossbar cb;
	HICANN::CrossbarRow row_cfg;

	cb.set(Coordinate::VLineOnHICANN(28), bg7.toHLineOnHICANN(), true); //BG 7(?)-> neuron0
	row_cfg = cb.get_row(bg7.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg7.toHLineOnHICANN(), left, row_cfg);

	cb.set(Coordinate::VLineOnHICANN(0), bg0.toHLineOnHICANN(), true); //neuron0 -> neuron1
	row_cfg =  cb.get_row(bg0.toHLineOnHICANN(), left);
	HICANN::set_crossbar_switch_row(h, bg0.toHLineOnHICANN(), left, row_cfg);
	
	// Syndriver Switch
	HICANN::SynapseSwitch sw;
	HICANN::SynapseSwitchRow srow_cfg;
	Coordinate::SynapseSwitchRowOnHICANN addr(Coordinate::SynapseSwitchRowOnHICANN(Y(111), left));

	sw.set(Coordinate::VLineOnHICANN(28), addr.line(), true); //BG 7(?) -> neuron0
	srow_cfg = sw.get_row(addr);
	HICANN::set_syndriver_switch_row(h, addr, srow_cfg);

	// Synapse Driver
	HICANN::SynapseDriver driver;
	driver.set_l1();
	driver[top].set_decoder(top, HICANN::DriverDecoder(0));
	driver[top].set_decoder(bottom, HICANN::DriverDecoder(0));
	driver[top].set_gmax_div(left, 1);
	driver[top].set_gmax_div(right, 1);
	driver[top].set_syn_in(left, 1);
	driver[top].set_syn_in(right, 0);
	driver[top].set_gmax(0);
	driver[bottom] = driver[top];
	
    // set_synapse_driver includes toggling the drivers DLL
	HICANN::set_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), driver); //BG -> neuron0
	
    // Synapses
	HICANN::DecoderDoubleRow drow;
	HICANN::WeightRow row;
	std::fill(drow[0].begin(), drow[0].end(), HICANN::SynapseDecoder(0xf)); //initialize with blocking values
	std::fill(drow[1].begin(), drow[1].end(), HICANN::SynapseDecoder(0xf));
	std::fill(row.begin(), row.end(), HICANN::SynapseWeight(0x0)); //initialize with lowest values

	//BG -> neuron0
	drow[0][0] = HICANN::SynapseDecoder(0);  //set left decoders to 0
	drow[1][0] = HICANN::SynapseDecoder(0);
	row[0] = HICANN::SynapseWeight(15); //set left weights to max, right weights to 0
	HICANN::set_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(Y(111), left), drow);
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Coordinate::SynapseDriverOnHICANN(Y(111), left), top), row); //top line of a driver
	HICANN::set_weights_row(h, Coordinate::SynapseRowOnHICANN(Coordinate::SynapseDriverOnHICANN(Y(111), left), bottom), row); //bottom line of a driver
	
    // Neurons
	HICANN::NeuronConfig nconf = HICANN::NeuronConfig(); //default should be OK
	HICANN::set_neuron_config(h, nconf);

	HICANN::Neuron neu = HICANN::Neuron();
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	neu.address(HICANN::L1Address(0));
	neu.activate_firing(true);
	neu.enable_fire_input(false);
	neu.enable_aout(false);

	nquad[Coordinate::NeuronOnQuad(X(0), Y(0))] = neu;
	
    HICANN::set_denmem_quad(h, HMF::Coordinate::QuadOnHICANN(0), nquad);

	//configure the fpga input
	
    FPGA::PulseEventContainer::container_type pc;
	
    using namespace ::HMF::Coordinate;
    using ::HMF::FPGA::PulseEvent;
	
    PulseEvent::dnc_address_t dnc_addr(h.coordinate().toDNCGlobal().toDNCOnFPGA());
    PulseEvent::chip_address_t hicann_addr(h.coordinate().toHICANNOnDNC().id());
	FPGA::PulseAddress pulse_address(dnc_addr, hicann_addr, PulseEvent::channel_t(0), HICANN::Neuron::address_t(0));
	//  500 Pulses with ISI = 500 clks (2000 ns)
	size_t isi = 500; //every 500 cycles
	size_t num_pulses = 500;
	for (size_t np = 0; np<num_pulses; ++np)
		pc.push_back(FPGA::PulseEvent(pulse_address, 1ULL * isi * (np + 1)));

	FPGA::write_playback_pulses(fpga, FPGA::PulseEventContainer(std::move(pc)), isi * num_pulses, 0);
	FPGA::prime_experiment(fpga);
	FPGA::start_experiment(fpga);
	
	//run and initialize the simulation
	std::cout << "Initilization done, run simulation: " << std::endl;
	fpga.runESS(5*1e4);
}

// loopback is not implemented in ESS yet
// TODO enable as soon as it is implemented
TEST_F( ESSTest, DISABLED_Layer2HICANNLoopbackTest)
{
	auto hicann_c = h.coordinate();
	auto hc = hicann_c.toHICANNOnDNC();
	auto d = hicann_c.toDNCOnFPGA();
	HICANN::reset(h);
	HICANN::init(h, false);
	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for(int j=0; j<8; j++){
		if (j%2) mer.config = HICANN::Merger::RIGHT_ONLY;
		else mer.config = HICANN::Merger::LEFT_ONLY;
		mer.slow = false;
		mer.loopback = !(j%2);
		mergers[Coordinate::DNCMergerOnHICANN(j)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

	//configure DNC<->HICANN links
	DNC::GbitReticle gbit = DNC::GbitReticle();
	HICANN::GbitLink link = HICANN::GbitLink();
	for (int i = 0; i < 8; i++){
		if (i%2) link.dirs[i] = HICANN::GbitLink::Direction::TO_DNC;
		else link.dirs[i] = HICANN::GbitLink::Direction::TO_HICANN;
	}
	gbit[hc] = link; //only HICANN0
	HICANN::set_gbit_link(h, link);
	DNC::set_hicann_directions(fpga, d, gbit);

	FPGA::PulseEventContainer::container_type pc;
	FPGA::PulseAddress pulse_address(
			d,
			hc,
			Coordinate::GbitLinkOnHICANN(0),
			HICANN::Neuron::address_t(63));
	//  500 Pulses with ISI = 500 clks (2000 ns)
	size_t isi = 50; //every 500 cycles
	size_t num_pulses = 100;
	size_t start_offset = 200;
	for (size_t np = 0; np<num_pulses; ++np)
		pc.push_back(FPGA::PulseEvent(pulse_address, 1ULL * isi * np + start_offset));
	FPGA::PulseEvent::spiketime_t const runtime_in_dnc_cycles = pc[pc.size()-1].getTime();
	uint64_t const run_time_in_us = runtime_in_dnc_cycles / FPGA::DNC_frequency_in_MHz;
	size_t const pulse_count = pc.size();

	// 63 fpga_clk cycles are 504 nano seconds
	FPGA::write_playback_pulses(
		fpga, FPGA::PulseEventContainer(std::move(pc)),
		runtime_in_dnc_cycles,
		/*fpga_hicann_delay=*/63);
	FPGA::prime_experiment(fpga);
	FPGA::start_experiment(fpga);
	fpga.runESS(run_time_in_us*1e3+1000);// last spike time + 1000 ns extra
	auto const received_data = FPGA::read_trace_pulses(fpga, 0 /* runtime not needed */).events;
	ASSERT_EQ(pulse_count, received_data.size());
}

// ECM (2016-07-28): Failing... Jenkins has to go blue!
TEST_F( ESSTest, DISABLED_RecordFromBEGTest)
{
	auto hicann_c = h.coordinate();
	auto hc = hicann_c.toHICANNOnDNC();
	auto d = hicann_c.toDNCOnFPGA();
	uint8_t pll_freq = 250; // Use PLL equal to dnc clock, such that bg_isi should match the recorded isi
	HICANN::reset(h, pll_freq);

	uint32_t bg_isi = 200;
	//configure background generators
	HICANN::BackgroundGeneratorArray bgarray;
	size_t num_active_begs = 1;
	HICANN::BkgRegularISI mode(bg_isi);
	for (size_t i = 0; i < num_active_begs; i++){
		bgarray[i].enable(true);
		bgarray[i].set_mode(mode);
		bgarray[i].seed(200);
		bgarray[i].address(HICANN::L1Address(0));
	}
	HICANN::set_background_generator(this->h, bgarray);

	//configure merger tree, phase
	HICANN::MergerTree tree; //default settings are OK
	HICANN::Phase phase = 0;
	HICANN::set_merger_tree(this->h, tree);
	HICANN::set_phase(this->h, phase);

	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for(int j=0; j<8; j++){
		mer.config = HICANN::Merger::RIGHT_ONLY;
		mer.slow = false;
		mer.loopback = false;
		mergers[Coordinate::DNCMergerOnHICANN(j)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

	//configure DNC<->HICANN links
	DNC::GbitReticle gbit = DNC::GbitReticle();
	HICANN::GbitLink link = HICANN::GbitLink();
	for (int i = 0; i < 8; i++){
		link.dirs[i] = HICANN::GbitLink::Direction::TO_DNC;
	}
	HICANN::set_gbit_link(h, link);
	gbit[hc] = link; //only the used HICANN
	DNC::set_hicann_directions(fpga, d, gbit);

	FPGA::prime_experiment(fpga);
	FPGA::start_experiment(fpga);
	fpga.runESS(1e5);
	auto const received_data = FPGA::read_trace_pulses(fpga, 0 /* runtime not needed */).events;
	size_t num_pulses = received_data.size();
	ASSERT_TRUE(num_pulses > 1);
	for (size_t ii=0; ii < num_pulses-1; ++ii){
		uint64_t rec_isi = received_data[ii+1].getTime() - received_data[ii].getTime();
		ASSERT_EQ(bg_isi, rec_isi);
	}
}
} //end namespace HMF
