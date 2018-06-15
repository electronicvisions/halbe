#include <gtest/gtest.h>

#include "hal/HMFUtil.h"
#include "hal/HICANN/FGControl.h"

using namespace std;
using namespace HMF::Coordinate;

namespace HMF {

//TEST(FGControl, SetTest) {
	//FGControl fg = FGControl();
	//typedef HICANN::shared_parameter g;
	//typedef HICANN::neuron_parameter n;

	//Coordinate::NeuronOnHICANN nrn;

	//fg.setShared(nrn.sharedFGBlock(), g::I_breset, 50);
	//fg.setNeuron(nrn, n::E_l, 40);

	//EXPECT_EQ(fg.getShared(nrn.sharedFGBlock(), g::I_breset), 50);
	//EXPECT_NE(fg.getNeuron(nrn, n::E_l), 0);
//}

//TEST(FGControl, SharedParameterTest) {
	//FGControl fg = FGControl();
	//typedef HICANN::shared_parameter g;
	//typedef HICANN::neuron_parameter n;

	////these parameters should only be accessed through the correct block, otherwise throw
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(0)), g::V_bout, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(0)), g::V_bexp, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(0)), g::V_clra, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(0)), g::V_clrc, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(1)), g::V_bexp, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(1)), g::V_bout, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(1)), g::V_clrc, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(1)), g::V_clra, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(2)), g::V_bout, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(2)), g::V_bexp, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(2)), g::V_clra, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(2)), g::V_clrc, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(3)), g::V_bexp, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(3)), g::V_bout, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(3)), g::V_clrc, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::FGBlockOnHICANN(Enum(3)), g::V_clra, 0); );

	//EXPECT_NO_THROW ( fg.setShared(Coordinate::NeuronOnHICANN(Enum(  0)).sharedFGBlock(), g::V_bout, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::NeuronOnHICANN(Enum(126)).sharedFGBlock(), g::V_bexp, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::NeuronOnHICANN(Enum(129)).sharedFGBlock(), g::V_clrc, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::NeuronOnHICANN(Enum(255)).sharedFGBlock(), g::V_clra, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::NeuronOnHICANN(Enum(256)).sharedFGBlock(), g::V_clra, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::NeuronOnHICANN(Enum(382)).sharedFGBlock(), g::V_clrc, 0); );
	//EXPECT_NO_THROW ( fg.setShared(Coordinate::NeuronOnHICANN(Enum(385)).sharedFGBlock(), g::V_bexp, 0); );
	//EXPECT_ANY_THROW( fg.setShared(Coordinate::NeuronOnHICANN(Enum(511)).sharedFGBlock(), g::V_bout, 0); );
//}

//TEST(FGControl, ConfigTest) {
	//FGControl fg = FGControl();
	//HICANN::FGConfig fgc = HICANN::FGConfig();

	//EXPECT_EQ(255, fgc.maxcycle.to_ulong()); //default value check
	//EXPECT_EQ(5, fgc.fg_biasn.to_ulong());

	//fgc.fg_biasn = 3;
	//fgc.acceleratorstep = 7;
	//fg.setConfig(Coordinate::FGBlockOnHICANN(Enum(0)),fgc);

	//EXPECT_EQ(3, fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(0))).fg_biasn.to_ulong());
	//EXPECT_EQ(7, fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(0))).acceleratorstep.to_ulong());

	//EXPECT_NE(3, fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(1))).fg_biasn.to_ulong());
	//EXPECT_NE(7, fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(2))).acceleratorstep.to_ulong());
//}

//TEST(FGControl, ExtractBlockTest) {
	//FGControl fg = FGControl();
	//HMF::HICANN::FGBlock block = HMF::HICANN::FGBlock();

	//fg.setShared(Coordinate::NeuronOnHICANN(Enum(20)).neuronFGBlock(), HICANN::shared_parameter::V_gmax0, 230);
	//fg.setShared(Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::shared_parameter::V_ccas, 231);
	//fg.setShared(Coordinate::NeuronOnHICANN(Enum(130)).neuronFGBlock(), HICANN::shared_parameter::V_br, 232);
	//fg.setShared(Coordinate::FGBlockOnHICANN(Enum(1)), HICANN::shared_parameter::V_reset, 233);
	//fg.setShared(Coordinate::NeuronOnHICANN(Enum(270)).neuronFGBlock(), HICANN::shared_parameter::V_stdf, 234);
	//fg.setShared(Coordinate::FGBlockOnHICANN(Enum(2)), HICANN::shared_parameter::V_bstdf, 235);
	//fg.setShared(Coordinate::NeuronOnHICANN(Enum(500)).neuronFGBlock(), HICANN::shared_parameter::V_tlow, 236);
	//fg.setShared(Coordinate::FGBlockOnHICANN(Enum(3)), HICANN::shared_parameter::V_thigh, 237);

	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(0)), HICANN::neuron_parameter::E_l, 240);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(100)), HICANN::neuron_parameter::E_syni, 241);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(150)), HICANN::neuron_parameter::E_synx, 242);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(200)), HICANN::neuron_parameter::I_bexp, 243);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(260)), HICANN::neuron_parameter::I_convi, 244);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(300)), HICANN::neuron_parameter::I_convx, 245);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(400)), HICANN::neuron_parameter::I_pl, 246);
	//fg.setNeuron(Coordinate::NeuronOnHICANN(Enum(500)), HICANN::neuron_parameter::I_radapt, 247);

	//HICANN::FGConfig tmp;
	//tmp = fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(0)));
	//tmp.currentwritetime = 1;
	//fg.setConfig(Coordinate::FGBlockOnHICANN(Enum(0)), tmp);
	//tmp = fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(1)));
	//tmp.voltagewritetime = 3;
	//fg.setConfig(Coordinate::FGBlockOnHICANN(Enum(1)), tmp);
	//tmp = fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(2)));
	//tmp.fg_bias          = 5;
	//fg.setConfig(Coordinate::FGBlockOnHICANN(Enum(2)), tmp);
	//tmp = fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(3)));
	//tmp.fg_biasn         = 7;
	//fg.setConfig(Coordinate::FGBlockOnHICANN(Enum(3)), tmp);

	//block = fg.extractBlock(Coordinate::FGBlockOnHICANN(Enum(0)));
	//EXPECT_EQ(230, block.fgarray[0][11]);
	//EXPECT_EQ(231, block.fgarray[0][23]);
	//EXPECT_EQ(240, block.fgarray[1][6]);
	//EXPECT_EQ(241, block.fgarray[101][18]);
	//EXPECT_EQ(1, block.config.currentwritetime.to_ulong());

	//block = fg.extractBlock(Coordinate::FGBlockOnHICANN(Enum(1)));
	//EXPECT_EQ(232, block.fgarray[0][21]);
	//EXPECT_EQ(233, block.fgarray[0][0]);
	//EXPECT_EQ(242, block.fgarray[106][0]);
	//EXPECT_EQ(243, block.fgarray[56][15]);
	//EXPECT_EQ(3, block.config.voltagewritetime.to_ulong());

	//block = fg.extractBlock(Coordinate::FGBlockOnHICANN(Enum(2)));
	//EXPECT_EQ(234, block.fgarray[0][14]);
	//EXPECT_EQ(235, block.fgarray[0][17]);
	//EXPECT_EQ(244, block.fgarray[5][3]);
	//EXPECT_EQ(245, block.fgarray[45][17]);
	//EXPECT_EQ(5, block.config.fg_bias.to_ulong());

	//block = fg.extractBlock(Coordinate::FGBlockOnHICANN(Enum(3)));
	//EXPECT_EQ(236, block.fgarray[0][10]);
	//EXPECT_EQ(237, block.fgarray[0][8]);
	//EXPECT_EQ(246, block.fgarray[112][9]);
	//EXPECT_EQ(247, block.fgarray[12][23]);
	//EXPECT_EQ(7, block.config.fg_biasn.to_ulong());
//}

//TEST(FGControl, ImportBlockTest) {
	//FGControl fg = FGControl();
	//std::array<HMF::HICANN::FGBlock, 4> block;

	//block[0].addr = Coordinate::FGBlockOnHICANN(Enum(0));
	//block[0].fgarray[0][11]          = 200;
	//block[0].fgarray[0][23]          = 201;
	//block[0].fgarray[1][6]           = 202;
	//block[0].fgarray[101][18]        = 203;
	//block[0].config.currentwritetime = 7;

	//block[1].addr = Coordinate::FGBlockOnHICANN(Enum(1));
	//block[1].fgarray[0][21]          = 204;
	//block[1].fgarray[0][0]           = 205;
	//block[1].fgarray[106][0]         = 206;
	//block[1].fgarray[56][15]         = 207;
	//block[1].config.voltagewritetime = 5;

	//block[2].addr = Coordinate::FGBlockOnHICANN(Enum(2));
	//block[2].fgarray[0][14]          = 208;
	//block[2].fgarray[0][17]          = 209;
	//block[2].fgarray[5][3]           = 210;
	//block[2].fgarray[45][17]         = 211;
	//block[2].config.fg_bias          = 3;

	//block[3].addr = Coordinate::FGBlockOnHICANN(Enum(3));
	//block[3].fgarray[0][10]          = 212;
	//block[3].fgarray[0][8]           = 213;
	//block[3].fgarray[112][9]         = 214;
	//block[3].fgarray[12][23]         = 215;
	//block[3].config.fg_biasn         = 1;

	//for (int i = 0; i < 4; i++) fg.importBlock(block[i]);

	//EXPECT_EQ(200 ,fg.getShared(Coordinate::NeuronOnHICANN(Enum(20)).neuronFGBlock(), HICANN::shared_parameter::V_gmax0));
	//EXPECT_EQ(201 ,fg.getShared(Coordinate::FGBlockOnHICANN(Enum(0)), HICANN::shared_parameter::V_ccas));
	//EXPECT_EQ(204 ,fg.getShared(Coordinate::NeuronOnHICANN(Enum(130)).neuronFGBlock(), HICANN::shared_parameter::V_br));
	//EXPECT_EQ(205 ,fg.getShared(Coordinate::FGBlockOnHICANN(Enum(1)), HICANN::shared_parameter::V_reset));
	//EXPECT_EQ(208 ,fg.getShared(Coordinate::NeuronOnHICANN(Enum(270)).neuronFGBlock(), HICANN::shared_parameter::V_stdf));
	//EXPECT_EQ(209 ,fg.getShared(Coordinate::FGBlockOnHICANN(Enum(2)), HICANN::shared_parameter::V_bstdf));
	//EXPECT_EQ(212 ,fg.getShared(Coordinate::NeuronOnHICANN(Enum(500)).neuronFGBlock(), HICANN::shared_parameter::V_tlow));
	//EXPECT_EQ(213 ,fg.getShared(Coordinate::FGBlockOnHICANN(Enum(3)), HICANN::shared_parameter::V_thigh));

	//EXPECT_EQ(202 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(0)), HICANN::neuron_parameter::E_l));
	//EXPECT_EQ(203 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(100)), HICANN::neuron_parameter::E_syni));
	//EXPECT_EQ(206 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(150)), HICANN::neuron_parameter::E_synx));
	//EXPECT_EQ(207 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(200)), HICANN::neuron_parameter::I_bexp));
	//EXPECT_EQ(210 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(260)), HICANN::neuron_parameter::I_convi));
	//EXPECT_EQ(211 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(300)), HICANN::neuron_parameter::I_convx));
	//EXPECT_EQ(214 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(400)), HICANN::neuron_parameter::I_pl));
	//EXPECT_EQ(215 ,fg.getNeuron(Coordinate::NeuronOnHICANN(Enum(500)), HICANN::neuron_parameter::I_radapt));

	//EXPECT_EQ(7 ,fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(0))).currentwritetime.to_ulong());
	//EXPECT_EQ(5 ,fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(1))).voltagewritetime.to_ulong());
	//EXPECT_EQ(3 ,fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(2))).fg_bias.to_ulong());
	//EXPECT_EQ(1 ,fg.getConfig(Coordinate::FGBlockOnHICANN(Enum(3))).fg_biasn.to_ulong());
//}

//TEST(FGControl, ExtractStimulusTest) {
	//FGControl fg = FGControl();

	//HICANN::Stimcurrent curr;
	//std::iota(curr.begin(), curr.end(), 5);
	//for (int i = 0; i < 4; i+=2){ //set only 2 blocks
		//Coordinate::FGBlockOnHICANN b = Coordinate::FGBlockOnHICANN(Enum(i));
		//fg.setStimcurrent(b, curr);
		//auto config = fg.getConfig(b);
		//config.pulselength = i+1;
		//fg.setConfig(b, config);
	//}

	//for (int i = 0; i < 4; i++){
		//HICANN::FGStimulus test = fg.extractStimulus(Coordinate::FGBlockOnHICANN(Enum(i)));

		//if (i%2) EXPECT_NE(curr, test.stimcurrent);
		//else EXPECT_EQ(curr, test.stimcurrent);

		//if (i%2) EXPECT_NE(i+1, test.pulselength.to_ulong());
		//else EXPECT_EQ(i+1, test.pulselength.to_ulong());
	//}
//}

//TEST(FGControl, ImportStimulusTest) {
	//FGControl fg = FGControl();
	//HICANN::FGStimulus stim = HICANN::FGStimulus();
	//HICANN::Stimcurrent curr;
	//std::iota(curr.begin(), curr.end(), 2);
	//stim.stimcurrent = curr;
	//stim.pulselength = 8;

	//for (int i = 1; i < 4; i+=2){ //set only 2 blocks
		//stim.addr = FGBlockOnHICANN(Enum(i));
		//fg.importStimulus(stim);
	//}

	//for (int i = 0; i < 4; i++){
		//FGBlockOnHICANN b {Enum{i}};
		//if (i%2) EXPECT_EQ(curr, fg.getStimcurrent(b));
		//else EXPECT_NE(curr, fg.getStimcurrent(b));

		//if (i%2) EXPECT_EQ(8, fg.getConfig(b).pulselength.to_ulong());
		//else EXPECT_NE(8, fg.getConfig(b).pulselength.to_ulong());
	//}
//}
} // namespace HMF
