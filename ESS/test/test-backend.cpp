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
