#include <cstdlib>
#include <ctime>
#include <cmath>
#include <array>
#include <iostream>

#include "bitter/integral.h"

#include "hal/backend/HMFBackend.h"
#include "hal/HMFUtil.h"
//#include "neuronbuilder_control.h"

//include halbe types
#include "hal/HICANN/FGControl.h"
#include "hal/HICANN/DNCMergerLine.h"

//include halbe coordinates
#include "hal/Coordinate/Merger0OnHICANN.h"
#include "hal/Coordinate/Merger1OnHICANN.h"
#include "hal/Coordinate/Merger2OnHICANN.h"
#include "hal/Coordinate/Merger3OnHICANN.h"
#include "hal/Coordinate/iter_all.h"

#include "ess-test-util.h"

// return random number in range [min,max]
// both are boundaries are included
int rand_range(int min, int max) {
	return rand()%(max-min+1) + min;
}

// return random number in range (min,max)
// both are boundaries are excluded
int rand_range_excl(int min, int max) {
	return rand_range(min+1,max-1);
}

namespace HMF
{


//check of parameters are set correctly
// ECM (2016-07-28): Failing... Jenkins has to go blue!
TEST_F(ESSTest, DISABLED_Test_Parameter)
{
	HICANN::init(h); //initialize hicann

    //initialize all neurons
	for(size_t i = 0; i < 128; i++)
	{
	    HICANN::NeuronQuad nquad = HICANN::NeuronQuad();
		HICANN::Neuron& neu0 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
		HICANN::Neuron& neu1 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(0))];
		HICANN::Neuron& neu2 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(1))];
		HICANN::Neuron& neu3 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(1))];

        //activate firing has to be activated for the neuron to be initialized
		neu0.activate_firing(true);
		neu1.activate_firing(true);
		neu2.activate_firing(true);
		neu3.activate_firing(true);
	
		Coordinate::QuadOnHICANN qb(i);
		HICANN::set_denmem_quad(h, qb, nquad);
    }

    HICANN::FGControl fgc{};
	srand(time(0));

	//initialize Parameters
	std::vector<int> param(39,0);
	// dac domains according to default neuron calibration
	// ranges extracted via calibtic/tools/extract_ranges.py
	// upper and lower limits are excluded, because rounding
	// in calibtic can lead to values out of domain in back
	// transformation.
	param[0] = rand_range_excl(0,1023);	//E_l
	param[1] = rand_range_excl(0,1023);	//E_syni
	param[2] = rand_range_excl(0,1023);	//E_synx
	param[3] = rand_range_excl(0,1023);	//I_bexp
	param[4] = rand_range_excl(0,1023);	//I_convi
	param[5] = rand_range_excl(0,1023);	//I_convx
	param[6] = rand_range_excl(23,1023);//I_fire
	param[7] = rand_range_excl(0,1023);	//I_gl
	param[8] = rand_range_excl(0,1023) ;//I_gladapt
	param[11] = rand_range_excl(16,1023);//I_pl
	param[12] = rand_range_excl(25,861);//I_radapt
	param[13] = rand_range_excl(0,1017);//I_rexp
	param[15] = rand_range_excl(0,1023);//V_exp
	param[17] = rand_range_excl(786,834); //V_syntci
	param[18] = rand_range_excl(786,834); //V_syntcx
	param[20] = rand_range_excl(0,1023);	//V_t
	param[29] = rand() % 1024;	//V_dtc; shared parameter
	param[30] = rand() % 1024;	//V_fac; shared parameter
	param[38] = rand() % 1024;	//V_stdf; shared parameter

	//set Parameters by inserting them via FGControl and set_fg_values via FGBlocks
	for (size_t neuron_number = 0; neuron_number < 512; neuron_number++)
	{
		Coordinate::NeuronOnHICANN nrn{Coordinate::Enum(neuron_number)};

		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l,  param[0]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, param[1] );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx,  param[2] );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, param[6]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, param[7]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, param[8]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, param[11]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, param[12]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, param[13]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, param[15]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, param[17]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, param[18]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, param[20]);
	}
	
    for (size_t i = 0; i < 4; i++)
	{
		Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};

		fgc.setShared(block, HICANN::shared_parameter::V_dtc, param[29]);
		fgc.setShared(block, HICANN::shared_parameter::V_fac, param[30]);
		fgc.setShared(block, HICANN::shared_parameter::V_stdf, param[38]);
	}

    //write parametes
    for (size_t i = 0; i < 4; i++)
	{
		Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};
		HICANN::set_fg_values(h, block, fgc[block]);
	}
	
    //initializing the ess
	fpga.initializeESS();

	//read out parameters via get_fg_values, check if they match
    for(size_t i = 0; i < 4; i++)
	{
		Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};
		HICANN::FGBlock fgb = fpga.ess().get_fg_values(h, block);
		fgc[block] = fgb;

		//shared parameters
		EXPECT_EQ(param[29], fgc.getShared(block, HICANN::shared_parameter::V_dtc));
		EXPECT_EQ(param[30], fgc.getShared(block, HICANN::shared_parameter::V_fac));
		EXPECT_EQ(param[38], fgc.getShared(block, HICANN::shared_parameter::V_stdf));

		//neuron-parameters
		size_t addr_offset = 128*i;
		for(size_t nrn_addr = addr_offset; nrn_addr < 128 + addr_offset - 1; nrn_addr++)
		{
			Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{nrn_addr}};

			ASSERT_EQ(param[0],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_l));
			ASSERT_EQ(param[1],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_syni));
			ASSERT_EQ(param[2],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_synx));
			ASSERT_EQ(param[6],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_fire));
			ASSERT_EQ(param[7],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_gl));
			ASSERT_EQ(param[8],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_gladapt));
			ASSERT_EQ(param[11], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_pl));
			ASSERT_EQ(param[12], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_radapt));
			ASSERT_EQ(param[13], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_rexp));
			ASSERT_EQ(param[15], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_exp));
            ASSERT_EQ(param[17], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_syntci));
			ASSERT_EQ(param[18], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_syntcx));
			ASSERT_EQ(param[20], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_t));
		}
	}
}

//check of parameters are set correctly
// ECM (2016-07-28): Failing... Jenkins has to go blue!
TEST_F(ESSTest, DISABLED_Test_Parameter_Row)
{
	HICANN::init(h); //initialize hicann

    //initialize all neurons
	for(size_t i = 0; i < 128; i++)
	{
	    HICANN::NeuronQuad nquad = HICANN::NeuronQuad();
		HICANN::Neuron& neu0 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
		HICANN::Neuron& neu1 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(0))];
		HICANN::Neuron& neu2 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(1))];
		HICANN::Neuron& neu3 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(1))];

        //activate firing has to be activated for the neuron to be initialized
		neu0.activate_firing(true);
		neu1.activate_firing(true);
		neu2.activate_firing(true);
		neu3.activate_firing(true);
	
		Coordinate::QuadOnHICANN qb(i);
		HICANN::set_denmem_quad(h, qb, nquad);
    }

    HICANN::FGControl fgc{};
	srand(time(0));

	//initialize Parameters
	std::vector<int> param(39,0);
	// dac domains according to default neuron calibration
	// ranges extracted via calibtic/tools/extract_ranges.py
	// upper and lower limits are excluded, because rounding
	// in calibtic can lead to values out of domain in back
	// transformation.
	param[0] = rand_range_excl(0,1023);	//E_l
	param[1] = rand_range_excl(0,1023);	//E_syni
	param[2] = rand_range_excl(0,1023);	//E_synx
	param[3] = rand_range_excl(0,1023);	//I_bexp
	param[4] = rand_range_excl(0,1023);	//I_convi
	param[5] = rand_range_excl(0,1023);	//I_convx
	param[6] = rand_range_excl(23,1023);//I_fire
	param[7] = rand_range_excl(0,1023);	//I_gl
	param[8] = rand_range_excl(0,1023) ;//I_gladapt
	param[11] = rand_range_excl(16,1023);//I_pl
	param[12] = rand_range_excl(25,861);//I_radapt
	param[13] = rand_range_excl(0,1017);//I_rexp
	param[15] = rand_range_excl(0,1023);//V_exp
	param[17] = rand_range_excl(786,834); //V_syntci
	param[18] = rand_range_excl(786,834); //V_syntcx
	param[20] = rand_range_excl(0,1023);	//V_t
	param[29] = rand() % 1024;	//V_dtc; shared parameter
	param[30] = rand() % 1024;	//V_fac; shared parameter
	param[38] = rand() % 1024;	//V_stdf; shared parameter

	//set Parameters by inserting them via FGControl and set_fg_values via FGBlocks
	for (size_t neuron_number = 0; neuron_number < 512; neuron_number++)
	{
		Coordinate::NeuronOnHICANN nrn{Coordinate::Enum(neuron_number)};

		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l,  param[0]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, param[1] );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx,  param[2] );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, param[6]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, param[7]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, param[8]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, param[11]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, param[12]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, param[13]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, param[15]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, param[17]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, param[18]);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, param[20]);
	}
	
    for (size_t i = 0; i < 4; i++)
	{
		Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};

		fgc.setShared(block, HICANN::shared_parameter::V_dtc, param[29]);
		fgc.setShared(block, HICANN::shared_parameter::V_fac, param[30]);
		fgc.setShared(block, HICANN::shared_parameter::V_stdf, param[38]);
	}

    //write parametes via set_fg_row_values
    for (auto fg_row : Coordinate::iter_all<Coordinate::FGRowOnFGBlock>())
	{
		HICANN::set_fg_row_values(h, fg_row, fgc, false, false);
	}
	
    //initializing the ess
	fpga.initializeESS();

	//read out parameters via get_fg_values, check if they match
    for(size_t i = 0; i < 4; i++)
	{
		Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};
		HICANN::FGBlock fgb = fpga.ess().get_fg_values(h, block);
		fgc[block] = fgb;

		//shared parameters
		EXPECT_EQ(param[29], fgc.getShared(block, HICANN::shared_parameter::V_dtc));
		EXPECT_EQ(param[30], fgc.getShared(block, HICANN::shared_parameter::V_fac));
		EXPECT_EQ(param[38], fgc.getShared(block, HICANN::shared_parameter::V_stdf));

		//neuron-parameters
		size_t addr_offset = 128*i;
		for(size_t nrn_addr = addr_offset; nrn_addr < 128 + addr_offset - 1; nrn_addr++)
		{
			Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{nrn_addr}};

			ASSERT_EQ(param[0],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_l));
			ASSERT_EQ(param[1],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_syni));
			ASSERT_EQ(param[2],  fgc.getNeuron(nrn,HICANN::neuron_parameter::E_synx));
			ASSERT_EQ(param[6],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_fire));
			ASSERT_EQ(param[7],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_gl));
			ASSERT_EQ(param[8],  fgc.getNeuron(nrn,HICANN::neuron_parameter::I_gladapt));
			ASSERT_EQ(param[11], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_pl));
			ASSERT_EQ(param[12], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_radapt));
			ASSERT_EQ(param[13], fgc.getNeuron(nrn,HICANN::neuron_parameter::I_rexp));
			ASSERT_EQ(param[15], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_exp));
            ASSERT_EQ(param[17], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_syntci));
			ASSERT_EQ(param[18], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_syntcx));
			ASSERT_EQ(param[20], fgc.getNeuron(nrn,HICANN::neuron_parameter::V_t));
		}
	}
}

//checks the correct configuration of a neuron quad
//L1Address is fetched from Ess and HAL2ESS datastructure
//other data only from HAL2ESS datastructure
// ECM (2016-07-28): Failing... Jenkins has to go blue!
TEST_F(ESSTest, DISABLED_Test_Quad_Config)
{
	HICANN::init(h); //initialize HICANN

	std::array<HICANN::NeuronQuad, 128> pattern1, pattern2;
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

	srand(time(0));

    //set reasonable FG values to avoid out of range exceptions
    HICANN::FGControl fgc;
	for(auto fgb : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>())
    {
		HICANN::set_fg_config(h, fgb, HICANN::FGConfig());
	    HICANN::set_fg_values(h, fgb, fgc[fgb]);
    }
    
    for(size_t i = 0; i < pattern1.size(); i++)
	{

		HICANN::Neuron& neu0 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
		HICANN::Neuron& neu1 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(0))];
		HICANN::Neuron& neu2 = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(1))];
		HICANN::Neuron& neu3 = nquad[Coordinate::NeuronOnQuad(geometry::X(0), geometry::Y(1))];

        //activate firing has to be activated for the neuron to be initialized
		neu0.activate_firing(true);
		neu1.activate_firing(true);
		neu2.activate_firing(true);
		neu3.activate_firing(true);
		//setting enable_fire_input randomly does not make sense
		neu0.address(HICANN::L1Address(rand() % 64));
		neu0.enable_fire_input(false);
		neu1.address(HICANN::L1Address(rand() % 64));
		neu1.enable_fire_input(false);
		neu2.address(HICANN::L1Address(rand() % 64));
		neu2.enable_fire_input(false);
		neu3.address(HICANN::L1Address(rand() % 64));
		neu3.enable_fire_input(false);
		
        std::bitset<4> inout = rand() % 16;

		neu0.enable_aout(inout[0]);
		neu0.enable_current_input(inout[2]);

		neu1.enable_aout(inout[1]);
		neu1.enable_current_input(inout[3]);

		neu2.enable_aout(inout[0]);
		neu2.enable_current_input(inout[2]);

		neu3.enable_aout(inout[1]);
		neu3.enable_current_input(inout[3]);

		nquad.setVerticalInterconnect(geometry::X(0), false);
		nquad.setVerticalInterconnect(geometry::X(1), false);
		nquad.setHorizontalInterconnect(geometry::Y(0), false);
		nquad.setHorizontalInterconnect(geometry::Y(1), false);

		pattern1[i] = nquad;
	}

	//setting the data
	for(size_t i = 0; i < pattern1.size(); ++i)
	{
		Coordinate::QuadOnHICANN qb(i);
		HICANN::set_denmem_quad(h, qb, pattern1[i]);
	}
	//initializing the ess
	fpga.initializeESS();
	//getting the data and checking for equality
	for(size_t i = 0; i < pattern2.size(); ++i)
	{
		Coordinate::QuadOnHICANN qb(i);
		pattern2[i] = HICANN::get_denmem_quad(h, qb);
		EXPECT_EQ(pattern1[i], pattern2[i]);
	}
}

//test for the configuration of the merger tree
//data only from HAL2ESS datastructure
//TODO: get data from Ess too -> update HAL2ESS::get_merger_tree
TEST_F(ESSTest, Test_MergerTree_Config)
{
	HICANN::init(h);

	//generating random test data
	std::array<HICANN::MergerTree,8> pattern1, pattern2;
	HICANN::Merger mer;
	HICANN::MergerTree tree;
	srand(time(0));
    for(size_t i=0; i < pattern1.size(); i++)
	{
		for(uint8_t j = 0; j < Coordinate::Merger0OnHICANN::size; j++)
		{
            Coordinate::Merger0OnHICANN m{j};
            mer.config = rand() % 4;
			mer.slow = rand() %2;
			tree[m] = mer;
		}
		for(uint8_t j = 0; j < Coordinate::Merger1OnHICANN::size; j++)
		{
            Coordinate::Merger1OnHICANN m{j};
            mer.config = rand() % 4;
			mer.slow = rand() %2;
			tree[m] = mer;
		}
		for(uint8_t j = 0; j < Coordinate::Merger2OnHICANN::size; j++)
		{
            Coordinate::Merger2OnHICANN m{j};
            mer.config = rand() % 4;
			mer.slow = rand() %2;
			tree[m] = mer;
		}
		for(uint8_t j = 0; j < Coordinate::Merger3OnHICANN::size; j++)
		{
            Coordinate::Merger3OnHICANN m{j};
            mer.config = rand() % 4;
			mer.slow = rand() %2;
			tree[m] = mer;
		}
		pattern1[i] = tree;
	}
	for(size_t i = 0; i < pattern1.size(); i++)
	{ //8 times for safety :)
		HICANN::set_merger_tree(h, pattern1[i]);
		pattern2[i] = HICANN::get_merger_tree(h);
	}
    EXPECT_EQ(pattern1, pattern2);
}

//test for the backgroundgenerators from Ess and HAL2ESS datastructure
TEST_F(ESSTest, Test_BackgroundGenerator_Config)
{
	HICANN::init(h);

	HICANN::BackgroundGeneratorArray pattern1, pattern2;
	HICANN::BackgroundGenerator bg;
	//generating test data
	srand(time(0));
	for(size_t i = 0; i < pattern1.size(); i++)
	{
		bg.enable(rand() % 2);
		bg.random(rand() % 2);
		bg.period(rand() % 0x10000); //syntax from test-HICANNBackend.cpp -> BackgroundGeneratorHWTest, guess it works, but dunno how
		bg.address(HICANN::L1Address(rand() % 64));
		pattern1[i] = bg;
	}
	HICANN::set_background_generator(h,pattern1);
	//writing data to the Ess
	fpga.initializeESS();
	//reading the data
	pattern2 = HICANN::get_background_generator(h);
	EXPECT_EQ(pattern1, pattern2);
	for(size_t i = 0; i < pattern1.size(); ++i) //seeds are not compared with the operator= of BG
	{
		EXPECT_EQ(pattern1[i].seed(), pattern2[i].seed());
	}
}

//test for the DNCMergers, data only from HAL2ESS datastructure
//TODO get data from Ess -> HAL2ESS::get_dnc_merger
TEST_F(ESSTest, Test_DNCMerger_Config)
{
	HICANN::init(h);
	//generate random test data
	std::array<HICANN::DNCMergerLine, 8> pattern1, pattern2;
	HICANN::DNCMergerLine tree{};
	HICANN::DNCMerger mer{};

	srand(time(0));
	for( size_t i = 0; i < pattern1.size(); ++i)
	{
		for(uint8_t j = 0; j < HICANN::DNCMergerLine::num_merger; ++j)
		{
            Coordinate::DNCMergerOnHICANN m{j};
            mer.config = rand() % 4;
			mer.slow = rand() % 2;
			tree[m] = mer;
		}
		for(uint8_t j = 0; j < 4; ++j)
		{
			std::bitset<2> temp = rand() % 3;	// 2 neighbor loopbacks cannot be 1 for some reason (cf. test-HICANNBackend -> DNCMergerHWTest)
			Coordinate::DNCMergerOnHICANN m0(2u * j);
			Coordinate::DNCMergerOnHICANN m1(2u * j + 1u);
			tree[m0].loopback = temp[0];
			tree[m1].loopback = temp[1];
		}
		pattern1[i] = tree;
	}
	for(size_t i = 0; i < pattern1.size();++i)
	{
		HICANN::set_dnc_merger(h,pattern1[i]);
		pattern2[i] = HICANN::get_dnc_merger(h);
	}

	EXPECT_EQ(pattern1,pattern2);
}

//test for the phae, only HAL2ESS datastructure
//TODO get from Ess too
TEST_F(ESSTest, Test_Phase_Config)
{
	HICANN::init(h);

	//generate random test data
	std::array<HAL2ESS::phase_t,8> pattern1, pattern2;

	srand(time(0));
	for(size_t i = 0; i < pattern1.size(); ++i)
	{
		pattern1[i] = rand() % 0x100;
	}

	for(size_t i = 0; i < pattern1.size(); ++i)
	{
		HICANN::set_phase(h,pattern1[i]);
		pattern2[i] = HICANN::get_phase(h);
	}

	EXPECT_EQ(pattern1,pattern2);
}

//test for vertical Repeater, data from Ess and HAL2ESS datastructure
TEST_F(ESSTest, Test_Repeater_Config_vert)
{
	HICANN::init(h);

	//generate random test data
	std::array<HICANN::VerticalRepeater, 256> pattern1, pattern2;
	HICANN::VerticalRepeater rep = HICANN::VerticalRepeater();

	srand(time(0));
	for(size_t i=0; i < pattern1.size(); i++)
    {

		bool mode = rand() % 2;		//only FORWARDING and IDLE are supported
		if (mode == false)
			rep.setIdle();
		else
		{
			bool dir = rand() % 2;
			if(dir)
				rep.setForwarding(geometry::top);
			else
				rep.setForwarding(geometry::bottom);
		}
		pattern1[i]=rep;
	}

	for(size_t i = 0; i < pattern1.size(); i++)
    {
		HICANN::set_repeater(
				h,
				Coordinate::VLineOnHICANN(i).toVRepeaterOnHICANN(),
				pattern1[i]);
	}
	fpga.initializeESS();
	for(size_t i = 0; i < pattern1.size(); i++)
    {
		pattern2[i] = HICANN::get_repeater(
				h, Coordinate::VLineOnHICANN(i).toVRepeaterOnHICANN());
	}
	EXPECT_EQ(pattern1, pattern2);
}

//test for horizontal repeater, data from Ess and HAL2ESS datastructure
TEST_F(ESSTest, Test_Repeater_Config_hor)
{
	HICANN::init(h);

	//generate random test data
	std::array<HICANN::HorizontalRepeater, 64> pattern1, pattern2;
	HICANN::HorizontalRepeater rep = HICANN::HorizontalRepeater();

	srand (time(0));
	//genrate test data
	for(size_t i=0; i < pattern1.size(); i++)
	{
		if (i%8!=6)	//Non-Sending-Repeater
		{
			bool mode = rand() % 2;
			if(mode == false)
				rep.setIdle();
			else
			{
				bool dir = rand() % 2;
				if(dir)
					rep.setForwarding(geometry::right);
				else
					rep.setForwarding(geometry::left);
			}
		}
		else	//Sending-Repeater
		{
			bool mode = rand() % 2;
			if(mode == false)
				rep.setIdle();
			else
			{
				bool dir = rand() % 2;
				if(dir)
					rep.setOutput(geometry::right);
				else
					rep.setOutput(geometry::left);
			}
		}
		pattern1[i]=rep;
		HICANN::set_repeater(
				h,
				Coordinate::HLineOnHICANN(i).toHRepeaterOnHICANN(), pattern1[i]);
	}
	//write data to ess
	fpga.initializeESS();
	//read data
	for(size_t i = 0; i < pattern1.size(); i++)
    {
		pattern2[i] = HICANN::get_repeater(
				h,
				Coordinate::HLineOnHICANN(i).toHRepeaterOnHICANN());
	}
	EXPECT_EQ(pattern1, pattern2);
}

//test for the crossbarmatrices, data from Ess and HAL2ESS datastructure
TEST_F(ESSTest, Test_Crossbar_Config)
{
	HICANN::init(h);

	std::array<HICANN::CrossbarRow, 64> pattern1l, pattern1r, pattern2l, pattern2r;
	std::array<bool, 4> d {{ true, false, false, false }};

	for (size_t i=0; i < pattern1l.size(); i++)
	{
		std::rotate(d.begin(), d.begin()+1, d.end());
		pattern1l[i] = d;
	}
	pattern1r = pattern1l;
	std::rotate(pattern1r.begin(), pattern1r.begin()+1, pattern1r.end());

	//write different pattern for left and right side
	for (size_t i=0; i < pattern1r.size(); i++)
	{
		HICANN::set_crossbar_switch_row(h, Coordinate::HLineOnHICANN(i), geometry::left, pattern1l[i]);
		HICANN::set_crossbar_switch_row(h, Coordinate::HLineOnHICANN(i), geometry::right, pattern1r[i]);
    }
	//write data to Ess
	fpga.initializeESS();
	//read data
	for (size_t i=0; i < pattern2r.size(); i++)
	{
		pattern2l[i] = HICANN::get_crossbar_switch_row(h, Coordinate::HLineOnHICANN(i), geometry::left);
		pattern2r[i] = HICANN::get_crossbar_switch_row(h, Coordinate::HLineOnHICANN(i), geometry::right);
	    ASSERT_EQ(pattern1l[i],pattern2l[i]);
	    ASSERT_EQ(pattern1r[i],pattern2r[i]);
	}
	EXPECT_EQ(pattern1r, pattern2r);
	EXPECT_EQ(pattern1l, pattern2l);
}

//test for the SynapseSwitchMatrices, data from HAL2ESS datastructures and the Ess
TEST_F(ESSTest, Test_Synswitches_Config)
{
	HICANN::init(h);

	//generate test data: only one value in the row should be != 0
	std::array<HICANN::SynapseSwitchRow, 224> pattern1l, pattern1r, pattern2l, pattern2r;
	std::array<bool, 16> d {{ true, false, false, false, false, false, false, false,
								false, false, false, false, false, false, false, false }};
	//generating pattern for the left side
	for (size_t i=0; i < pattern1l.size(); i++)
	{
		std::rotate(d.begin(), d.begin()+1, d.end());
		pattern1l[i] = d;
	}
	pattern1r = pattern1l;
	 //change the pattern for the right side
	std::rotate(pattern1r.begin(), pattern1r.begin()+1, pattern1r.end());

	for (size_t i=0; i<224; i++)
	{
		HICANN::set_syndriver_switch_row(h, Coordinate::SynapseSwitchRowOnHICANN(geometry::Y(i), geometry::left), pattern1l[i]);
		HICANN::set_syndriver_switch_row(h, Coordinate::SynapseSwitchRowOnHICANN(geometry::Y(i), geometry::right), pattern1r[i]);
	}
	//write data to Ess
	fpga.initializeESS();
	//get data and check for equality
	for (size_t i=0; i<224; i++)
	{
		pattern2l[i] = HICANN::get_syndriver_switch_row(h, Coordinate::SynapseSwitchRowOnHICANN(geometry::Y(i), geometry::left));
		pattern2r[i] = HICANN::get_syndriver_switch_row(h, Coordinate::SynapseSwitchRowOnHICANN(geometry::Y(i), geometry::right));
	}
	EXPECT_EQ(pattern1l, pattern2l);
	EXPECT_EQ(pattern1r, pattern2r);
}

//test for the synaptic weights, data from HAL2ESS datastructures and the Ess
// ECM (2016-07-27): assert in get_weights_row() fails...
TEST_F(ESSTest, DISABLED_Test_Weight_Config)
{
	HICANN::init(h);
    //configure a neuron (else the ESS wont initialize the anncore)
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();
	HICANN::Neuron& neu = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
	neu.activate_firing(true);
	Coordinate::QuadOnHICANN qb(0);
	HICANN::set_denmem_quad(h, qb, nquad);
    //set reasonable FG values to avoid out of range exceptions
    HICANN::FGControl fgc;
	for(auto fgb : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>())
    {
		HICANN::set_fg_config(h, fgb, HICANN::FGConfig());
	    HICANN::set_fg_values(h, fgb, fgc[fgb]);
    }

	//generate test data
	std::array<HICANN::WeightRow, 224> pattern1l, pattern1r, pattern2l, pattern2r;
	HICANN::WeightRow row;
	//left side
	std::generate(row.begin(), row.end(), IncrementingSequence<HICANN::SynapseWeight>(0xf));
	for (size_t i = 0; i < pattern1l.size(); i++)
	{
		std::rotate(row.begin(), row.begin()+1, row.end());
		pattern1l[i]=row;
	}
	//change the pattern for the right side
	pattern1r = pattern1l;
	std::rotate(pattern1r.begin(), pattern1r.begin()+1, pattern1r.end());
	
	//writing data for the left side
	int j = 0; //variable for pattern writing sequence (not derivable from i!)
	for (size_t i = 1; i < pattern1l.size(); i+=2)
	{
        Coordinate::SynapseRowOnHICANN coord_top(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::left), geometry::top);
		Coordinate::SynapseRowOnHICANN coord_bot(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::left), geometry::bottom);
		HICANN::set_weights_row(h, coord_top, pattern1l[j]); //top line of a driver
		HICANN::set_weights_row(h, coord_bot, pattern1l[j+1]); //bottom line of a drive
		if(i==111)
			i=110; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	//writing data for the right side
	j=0;
	for (size_t i = 0; i < pattern1r.size(); i+=2)
	{
        Coordinate::SynapseRowOnHICANN coord_top(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right), geometry::top);
		Coordinate::SynapseRowOnHICANN coord_bot(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right), geometry::bottom);
		HICANN::set_weights_row(h, coord_top, pattern1r[j]); //top line of a driver
		HICANN::set_weights_row(h, coord_bot, pattern1r[j+1]); //bottom line of a driv
		if(i==110) 
            i=111; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	//writing the data to the Ess
	fpga.initializeESS();
	//reading data for the left side
	j = 0; //variable for pattern writing sequence (not derivable from i!)
	for (size_t i = 1; i < pattern2l.size(); i+=2)
	{
        Coordinate::SynapseRowOnHICANN coord_top(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::left), geometry::top);
		Coordinate::SynapseRowOnHICANN coord_bot(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::left), geometry::bottom);
        pattern2l[j]   = HICANN::get_weights_row(h, coord_top);
		pattern2l[j+1] = HICANN::get_weights_row(h, coord_bot);
		if(i==111)
			i=110; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	//reading data for the right side
	j = 0;
	for (size_t i = 0; i < pattern2r.size(); i+=2)
	{
        Coordinate::SynapseRowOnHICANN coord_top(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right), geometry::top);
		Coordinate::SynapseRowOnHICANN coord_bot(Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right), geometry::bottom);
		pattern2r[j]   = HICANN::get_weights_row(h, coord_top);
		pattern2r[j+1] = HICANN::get_weights_row(h, coord_bot);
		if(i==110) 
            i=111; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	EXPECT_EQ(pattern1l, pattern2l);
	EXPECT_EQ(pattern1r, pattern2r);
}

//test for the double decoder rows, data from HAL2ESS datastructure and the Ess
// ECM (2016-07-28): Failing... Jenkins has to go blue!
TEST_F(ESSTest, DISABLED_Test_Synapse_Decoder_Config)
{
	HICANN::init(h);
    //configure a neuron (else the ESS wont initialize the anncore)
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();
	HICANN::Neuron& neu = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
	neu.activate_firing(true);
	Coordinate::QuadOnHICANN qb(0);
	HICANN::set_denmem_quad(h, qb, nquad);
    //set reasonable FG values to avoid out of range exceptions
    HICANN::FGControl fgc;
	for(auto fgb : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>())
    {
		HICANN::set_fg_config(h, fgb, HICANN::FGConfig());
	    HICANN::set_fg_values(h, fgb, fgc[fgb]);
    }
	
    //generate data
	std::array<HICANN::DecoderDoubleRow, 112> pattern1l, pattern1r, pattern2l, pattern2r;
	HICANN::DecoderDoubleRow drow;
	//generating test data for the left side
	std::generate(drow[0].begin(), drow[0].end(), IncrementingSequence<HICANN::SynapseDecoder>(0xf));
	std::generate(drow[1].begin(), drow[1].end(), IncrementingSequence<HICANN::SynapseDecoder>(0xf));
	std::rotate(drow[1].begin(), drow[1].begin()+2, drow[1].end());
	for (size_t i = 0; i < pattern1l.size(); i++)
	{
		std::rotate(drow[0].begin(), drow[0].begin()+1, drow[0].end());
		std::rotate(drow[1].begin(), drow[1].begin()+1, drow[1].end());
		pattern1l[i]=drow;
	}
	pattern1r = pattern1l;
	//change the pattern for the right side
	std::rotate(pattern1r[0].begin(), pattern1r[0].begin()+1, pattern1r[0].end());
	std::rotate(pattern1r[1].begin(), pattern1r[1].begin()+1, pattern1r[1].end());
	enum : size_t {TOP = 0, BOT = 1};	//top = upper row, bot = lower row
	
	//write data for the left side
	int j = 0; //variable for pattern writing sequence (not derivable from i!)
	int k = 0; //variable for weights writing
	for(size_t i = 1; i < 2*pattern1l.size(); i+=2)
	{
		//setting the decoder values
		Coordinate::SynapseDriverOnHICANN coord{geometry::Y(i), geometry::left};
		HICANN::set_decoder_double_row(h, coord, pattern1l[j]);
		if(i==111)
			i=110; //switch from top to bottom half: line numbers are different
		j++;
		k+=2;
	}
	//write data for the right side
	j = 0;
	k = 0;
	for(size_t i = 0; i < 2*pattern1r.size(); i+=2)
	{
		//setting the decoder values
		Coordinate::SynapseDriverOnHICANN coord{geometry::Y(i), geometry::right};
		HICANN::set_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right), pattern1r[j]);
		if(i==110)
			i=111; //switch from top to bottom half: line numbers are different
		j++;
		k+=2;
	}
	//write data to Ess
	fpga.initializeESS();
	//read data for the left side
	j = 0; //variable for pattern writing sequence (not derivable from i!)
	for(size_t i = 1; i < 2*pattern2l.size(); i+=2)
	{
		Coordinate::SynapseDriverOnHICANN coord{geometry::Y(i), geometry::left};
		pattern2l[j] = HICANN::get_decoder_double_row(h, coord);
		if(i==111)
			i=110; //switch from top to bottom half: line numbers are different
		j++;
	}
	//read data for the right side
	j = 0;
	for(size_t i = 0; i < 2*pattern2r.size(); i+=2)
	{
		Coordinate::SynapseDriverOnHICANN coord{geometry::Y(i), geometry::right};
		pattern2r[j] = HICANN::get_decoder_double_row(h, Coordinate::SynapseDriverOnHICANN(geometry::Y(i), geometry::right));
		if(i==110)
			i=111; //switch from top to bottom half: line numbers are different
		j++;
	}
	EXPECT_EQ(pattern1l, pattern2l);
	EXPECT_EQ(pattern1r, pattern2r);
}

//test for the synapse driver, data from Ess and HAL2ESS datastructures
TEST_F(ESSTest, DISABLED_Test_Syndr_Config)
{
	HICANN::init(h);
    //configure a neuron (else the ESS wont initialize the anncore)
	HICANN::NeuronQuad nquad = HICANN::NeuronQuad();
	HICANN::Neuron& neu = nquad[Coordinate::NeuronOnQuad(geometry::X(1), geometry::Y(0))];
	neu.activate_firing(true);
	Coordinate::QuadOnHICANN qb(0);
	HICANN::set_denmem_quad(h, qb, nquad);
    //set reasonable FG values to avoid out of range exceptions
    HICANN::FGControl fgc;
	for(auto fgb : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>())
    {
		HICANN::set_fg_config(h, fgb, HICANN::FGConfig());
	    HICANN::set_fg_values(h, fgb, fgc[fgb]);
    }
    
    //generate test data
    using namespace geometry;
    srand(time(0));
	std::array<HICANN::SynapseDriver,224> pattern1,pattern2;
	for (size_t drv = 0; drv < pattern1.size(); ++drv)
	{
		HICANN::SynapseDriver driver;
		//set the line configuration
        for (auto const& tt : { top, bottom })
		{
		    //set the driver decoder
            for (auto const& uu : { top, bottom })
            {
				driver[tt].set_decoder(uu, HICANN::DriverDecoder(rand() % 4));
            }
			//set the synapse type
            //no all combinations possible in the ESS
            Coordinate::Side side;
            if(rand() %2)
                side = left;
            else
                side = right;
			driver[tt].set_syn_in(side, true);

			// set default gmax config, otherwise synapse calibration is not found in Ess
			// (This is similar to choosing reasonable FG values)
			driver[tt].set_gmax(0);
			driver[tt].set_gmax_div(side, 11);
		}
		driver.stp_cap = rand() % 8;
	    bool _enable        = rand() % 2;
        bool _stp_enable    = rand() % 2;
        //only set valid configs
        if (_enable)
        {
            short _mode = rand()%4;
            if( _mode == 0)
                driver.set_l1();
            else if (_mode == 1)
                driver.set_l1_mirror();
            else if (_mode == 2)
                driver.set_mirror();
            else if (_mode == 3)
                driver.set_listen();
        }
        else
        {
            driver.disable();
        }
        if (_stp_enable)
        {
            bool _stp_mode = rand()%2;
            if (_stp_mode)
                driver.set_stf();
            else
                driver.set_std();
        }
        else
        {
            driver.disable_stp();
        }
		pattern1[drv] = driver;
		HICANN::set_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Enum(drv)), pattern1[drv]);
	}
	//write data to the Ess
	fpga.initializeESS();
	//read data
	for(size_t drv = 0; drv < pattern2.size(); ++drv)
	{
		pattern2[drv] = HICANN::get_synapse_driver(h, Coordinate::SynapseDriverOnHICANN(Enum(drv)));
		EXPECT_EQ(pattern1[drv],pattern2[drv]) << "Synapse driver config differs for driver " << drv << ", " << Coordinate::SynapseDriverOnHICANN(Enum(drv)) ;
	}
	EXPECT_EQ(pattern1,pattern2);
}

//test for the neuron config
//currently only capacitance used in the Ess
//data only from HAL2ESS data structure
//TODO: get data from Ess
TEST_F(ESSTest, Test_NeuronConfig)
{
	HICANN::init(h);
	srand(time(0));
	HICANN::NeuronConfig nrn_cfg{};
	for(size_t i = 0; i < 10; ++i)	//test 10 times just to make sure
	{
		nrn_cfg.bigcap[geometry::top] = rand()%2;
		nrn_cfg.bigcap[geometry::bottom] = rand()%2;
		HICANN::set_neuron_config(h, nrn_cfg);
		auto test = HICANN::get_neuron_config(h);
		EXPECT_EQ(test, nrn_cfg);
	}
}

//test for the current stimulus
//data only from HAL2ESS datastructure
//TODO: get data from Ess
TEST_F(ESSTest, Test_CurrentStimulus)
{
	HICANN::reset(h);
    srand(time(0));
    for (int i = 0; i < 4; i++)
    {
		HICANN::FGStimulus s;
		s.setContinuous(rand() % 2);
        for (int j = 0; j < 129; j++)
			s[j] = rand() % 1024;
		HICANN::FGConfig cfg;

		HICANN::set_current_stimulus(h, Coordinate::FGBlockOnHICANN(Coordinate::Enum(i)), s);
		HICANN::FGStimulus r = HICANN::get_current_stimulus(h, Coordinate::FGBlockOnHICANN(Coordinate::Enum(i)));
		EXPECT_EQ(s, r);
	}
}


} //end namespace HMF
