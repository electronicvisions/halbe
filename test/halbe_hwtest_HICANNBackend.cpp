#include <chrono>

#include "test/hwtest.h"
#include "test/NeuronBuilderHelper.h"
#include "hal/backend/HICANNBackendHelper.h"
#include "hal/Coordinate/iter_all.h"

//necessary for the direct control of low-level functions of the hardware
#include "reticle_control.h"       //reticle control class
#include "fpga_control.h"          //FPGA control class
#include "dnc_control.h"           //DNC control class
#include "hicann_ctrl.h"           //HICANN control class
#include "synapse_control.h"       //synapse control class
#include "l1switch_control.h"      //layer 1 switch control class
#include "repeater_control.h"      //repeater control class
#include "neuron_control.h"        //neuron control class (merger, background genarators)
#include "neuronbuilder_control.h" //neuron builder control class
#include "fg_control.h"            //floating gate control
#include "spl1_control.h"          //spl1 control class
#include "dncif_control.h"         //DNC interface control

#define RET reticle->hicann[this->jtag_addr()]
#define HCL1 facets::HicannCtrl::L1Switch
#define HCFG facets::HicannCtrl::FG
#define HCREP facets::HicannCtrl::Repeater
#define HCSYN facets::HicannCtrl::Synapse

using namespace HMF::Coordinate;
using namespace facets;

using std::endl;
using std::cout;
using std::vector;

namespace HMF {

template <typename T>
class HICANNBackendTest : public ::TypedHWTest<T> {
protected:
	void FIFOtest(bool loop);
	size_t L1TransmissionTest(RepeaterBlockOnHICANN block);
};
TYPED_TEST_CASE(HICANNBackendTest, HandleTypes);


TYPED_TEST(HICANNBackendTest, CreateReticleHWTest) {
	//first call of PowerBackend: Vertical Setup / Wafer is initialized
	// TODO fix?
	EXPECT_GETTER_NE(this->get_reticle(), nullptr);
}

TYPED_TEST(HICANNBackendTest, HICANNInitHWTest) {
	HICANN::init(this->h, false);
}

TYPED_TEST(HICANNBackendTest, CrossbarHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data: only one value in the row should be != 0
	std::array<HICANN::CrossbarRow, 64> pattern1, pattern2;
	std::array<bool, 4> data {{ true, false, false, false }};

	for (int i=0; i<64; i++) {
		std::rotate(data.begin(), data.begin()+1, data.end());
		pattern1[i] = data;
	}

	for (int i=0; i<64; i++){
		HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(i), left, pattern1[i]);
		pattern2[i] = HICANN::get_crossbar_switch_row(this->h, HLineOnHICANN(i), left);
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	//change the pattern before the second test just in case...
	std::rotate(pattern1.begin(), pattern1.begin()+1, pattern1.end());

	for (int i=0; i<64; i++){
		HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(i), right, pattern1[i]);
		pattern2[i] = HICANN::get_crossbar_switch_row(this->h, HLineOnHICANN(i), right);
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).print_config();
	//~ RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).print_config();

	// @AK: ok?
	//RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).reset();
	//RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, SynapseSwitchHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data: only one value in the row should be != 0
	std::array<HICANN::SynapseSwitchRow, 224> pattern1, pattern2;
	std::array<bool, 16> data {{ true, false, false, false, false, false, false, false,
								false, false, false, false, false, false, false, false }};

	for (int i=0; i<224; i++) {
		std::rotate(data.begin(), data.begin()+1, data.end());
		pattern1[i] = data;
	}

	for (int i=0; i<224; i++){
		HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(i), left), pattern1[i]);
		pattern2[i] = HICANN::get_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(i), left));
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	 //change the pattern before the second test just in case...
	std::rotate(pattern1.begin(), pattern1.begin()+1, pattern1.end());

	for (int i=0; i<224; i++)	{
		HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(i), right), pattern1[i]);
		pattern2[i] = HICANN::get_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(i), right));
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getLC(HCL1::L1SWITCH_TOP_LEFT).print_config();
	//~ RET->getLC(HCL1::L1SWITCH_BOTTOM_LEFT).print_config();
	//~ RET->getLC(HCL1::L1SWITCH_TOP_RIGHT).print_config();
	//~ RET->getLC(HCL1::L1SWITCH_BOTTOM_RIGHT).print_config();
	// @AK: ok?
	// RET->getLC(HCL1::L1SWITCH_TOP_LEFT).reset();
	// RET->getLC(HCL1::L1SWITCH_BOTTOM_LEFT).reset();
	// RET->getLC(HCL1::L1SWITCH_TOP_RIGHT).reset();
	// RET->getLC(HCL1::L1SWITCH_BOTTOM_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, DISABLED_SynapseWeightHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data
	std::array<HICANN::WeightRow, 224> pattern1, pattern2;
	HICANN::WeightRow row;
	std::generate(row.begin(), row.end(), IncrementingSequence<HICANN::SynapseWeight>(0xf));

	for (int i=0; i<224; i++) {
		std::rotate(row.begin(), row.begin()+1, row.end());
		pattern1[i]=row;
	}

	int j=0; //variable for pattern writing sequence (not derivable from i!)
	for (int i=1; i<224; i+=2){
		HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), left), top), pattern1[j]); //top line of a driver
		HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), left), bottom), pattern1[j+1]); //bottom line of a driver
		pattern2[j]   = HICANN::get_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), left), top));
		pattern2[j+1] = HICANN::get_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), left), bottom));
		if(i==111) i=110; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	//change the pattern before the second test just in case...
	std::rotate(pattern1.begin(), pattern1.begin()+1, pattern1.end());
	j=0;

	for (int i=0; i<224; i+=2){
		HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), right), top), pattern1[j]); //top line of a driver
		HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), right), bottom), pattern1[j+1]); //bottom line of a driver
		pattern2[j]   = HICANN::get_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), right), top));
		pattern2[j+1] = HICANN::get_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(i), right), bottom));
		if(i==110) i=111; //switch from top to bottom half: line numbers are different
		j+=2;
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	//~ ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).reset_all();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).reset_all();
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).print_weight();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).print_weight();
}

TYPED_TEST(HICANNBackendTest, DISABLED_SynapseDecoderHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data
	std::array<HICANN::DecoderDoubleRow, 112> pattern1, pattern2;
	HICANN::DecoderDoubleRow drow;

	std::generate(drow[0].begin(), drow[0].end(), IncrementingSequence<HICANN::SynapseDecoder>(0xf));
	std::generate(drow[1].begin(), drow[1].end(), IncrementingSequence<HICANN::SynapseDecoder>(0xf));
	std::rotate(drow[1].begin(), drow[1].begin()+2, drow[1].end());

	for (int i=0; i<112; i++) {
		std::rotate(drow[0].begin(), drow[0].begin()+1, drow[0].end());
		std::rotate(drow[1].begin(), drow[1].begin()+1, drow[1].end());
		pattern1[i]=drow;
	}

	int j=0; //variable for pattern writing sequence (not derivable from i!)
	for (int i=1; i<224; i+=2){
		HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(i), left), pattern1[j]);
		pattern2[j] = HICANN::get_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(i), left));
		if(i==111) i=110; //switch from top to bottom half: line numbers are different
		j++;
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	//change the pattern before the second test just in case...
	std::rotate(pattern1[0].begin(), pattern1[0].begin()+1, pattern1[0].end());
	std::rotate(pattern1[1].begin(), pattern1[1].begin()+1, pattern1[1].end());
	j=0;

	for (int i=0; i<224; i+=2){
		HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(i), right), pattern1[j]);
		pattern2[j] = HICANN::get_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(i), right));
		if(i==110) i=111; //switch from top to bottom half: line numbers are different
		j++;
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	//~ ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).reset_all();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).reset_all();
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).print_decoder();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).print_decoder();
}

TYPED_TEST(HICANNBackendTest, SynapseDriverHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	size_t const N = 2;
	for (size_t iter=0; iter<N; ++iter)
	{
		for (size_t drv=0; drv<224; ++drv)
		{
			HICANN::SynapseDriver driver;

			for (auto const& tt : { top, bottom })
			{
				for (auto const& uu : { top, bottom })
					driver[tt].set_decoder(uu, HICANN::DriverDecoder(rand() % 4));

				for (auto const& uu : { left, right })
				{
					driver[tt].set_gmax_div(uu, rand() % 16);
					driver[tt].set_syn_in(uu, rand() % 2);
				}

				driver[tt].set_gmax(rand() % 4);
			}

			driver.stp_cap              = rand() % 8;
			driver.enable               = rand() % 2;
			driver.locin                = rand() % 2;
			driver.connect_neighbor     = rand() % 2;
			driver.stp_enable           = rand() % 2;
			driver.stp_mode             = rand() % 2;


			HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Enum(drv)), driver);
			auto ret = HICANN::get_synapse_driver(this->h, SynapseDriverOnHICANN(Enum(drv)));
			ASSERT_GETTER_EQ(driver, ret);
		}
	}

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).print_config();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).print_config();
	// RET->getSC(HCSYN::SYNAPSE_TOP).reset_drivers();
	// RET->getSC(HCSYN::SYNAPSE_BOTTOM).reset_drivers();
}

TYPED_TEST(HICANNBackendTest, STDPConfigHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data
	HICANN::STDPControl pattern1, pattern2;

	pattern1.eval = HICANN::STDPEval();
	pattern1.lut.set_defaults();
	pattern1.lut.causal[7] = 15;
	pattern1.without_reset = true;
	pattern1.read_causal = true;
	pattern1.read_acausal = false;
	pattern1.continuous_autoupdate = false;
	pattern1.set_first_row(2);
	pattern1.set_last_row(8);
	pattern1.timing.predel = 8;
	pattern1.timing.outdel = 8;

	HICANN::set_stdp_config(this->h, top, pattern1);
	pattern2 = HICANN::get_stdp_config(this->h, top);

	EXPECT_GETTER_EQ(pattern1, pattern2);
}

static size_t hamming_distance(HICANN::NeuronQuad const& a, HICANN::NeuronQuad const& b)
{
	size_t distance = 0;
	for (size_t ii=0; ii<4; ++ii)
	{
		auto const _a = denmen_quad_formatter(NeuronOnQuad(Enum(ii)), a);
		auto const _b = denmen_quad_formatter(NeuronOnQuad(Enum(ii)), b);

		for (size_t jj=0; jj<_a.size(); ++jj)
		{
			if (_a[jj] != _b[jj])
				++distance;
		}
	}
	return distance;
}

TYPED_TEST(HICANNBackendTest, DenmemQuadHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	srand (time(NULL));

	for (int i=0; i<128; i++){ //128 quads on a HICANn
		HICANN::NeuronQuad nquad;

		HICANN::Neuron& neu0 = nquad[NeuronOnQuad(X(1), Y(0))];
		HICANN::Neuron& neu1 = nquad[NeuronOnQuad(X(0), Y(0))];
		HICANN::Neuron& neu2 = nquad[NeuronOnQuad(X(1), Y(1))];
		HICANN::Neuron& neu3 = nquad[NeuronOnQuad(X(0), Y(1))];

		neu0.address(HICANN::L1Address(rand() % 64));
		neu0.activate_firing(rand() % 2);
		neu0.enable_spl1_output(rand() % 2);
		neu0.enable_fire_input(rand() % 2);
		neu1.address(HICANN::L1Address(rand() % 64));
		neu1.activate_firing(rand() % 2);
		neu1.enable_spl1_output(rand() % 2);
		neu1.enable_fire_input(rand() % 2);
		neu2.address(HICANN::L1Address(rand() % 64));
		neu2.activate_firing(rand() % 2);
		neu2.enable_spl1_output(rand() % 2);
		neu2.enable_fire_input(rand() % 2);
		neu3.address(HICANN::L1Address(rand() % 64));
		neu3.activate_firing(rand() % 2);
		neu3.enable_spl1_output(rand() % 2);
		neu3.enable_fire_input(rand() % 2);

		//try different random configurations for IObits until they are feasible
		std::bitset<4> inout;
		while (true)
		{
			try {
				inout = rand() % 16;
				HICANN::denmem_iomap(inout);
				break;
			} catch (std::runtime_error &) {}
		}

		neu0.enable_aout(inout[0]);
		neu0.enable_current_input(inout[2]);

		neu1.enable_aout(inout[1]);
		neu1.enable_current_input(inout[3]);

		neu2.enable_aout(inout[0]);
		neu2.enable_current_input(inout[2]);

		neu3.enable_aout(inout[1]);
		neu3.enable_current_input(inout[3]);

		nquad.setVerticalInterconnect(X(0), rand() % 2);
		nquad.setVerticalInterconnect(X(1), rand() % 2);
		nquad.setHorizontalInterconnect(Y(0), rand() % 2);
		nquad.setHorizontalInterconnect(Y(1), rand() % 2);

		HICANN::set_denmem_quad(this->h, QuadOnHICANN(i), nquad);


		// because reading back bits from NeuronBuilder is unreliable
		// (https://brainscales-r.kip.uni-heidelberg.de/issues/81V), we read
		// multiple times.

		std::array<HICANN::NeuronQuad, 3> res = {{ nquad }};
		size_t iter = 1000;
		do {
			for (auto& entry : res)
				entry = HICANN::get_denmem_quad(this->h, QuadOnHICANN(i));

			--iter;

			// test that not too many bits flipped. Flipping bits are unavoidable
			// in the current hardware system. The only thing we can make test, that
			// it doesn't happen to often and if it happens, that the hamming
			// distance is not to great.
			// https://brainscales-r.kip.uni-heidelberg.de/issues/81V
			for (size_t kk=0 ; kk<res.size(); ++kk)
				EXPECT_GETTER_GE(5, hamming_distance(res.at((kk+1)%res.size()), res.at(kk%res.size())));
		} while (!std::equal(res.begin(), res.end(), res.begin()) && iter);
		ASSERT_GETTER_TRUE(std::equal(res.begin(), res.end(), res.begin())) << QuadOnHICANN(i);
	}

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getNBC().print_config();
	// RET->getNBC().initzeros();
}

TYPED_TEST(HICANNBackendTest, NeuronConfigHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	HICANN::NeuronConfig pattern1 = HICANN::NeuronConfig(), pattern2 = HICANN::NeuronConfig();

	srand (time(NULL));
	pattern1.slow_I_gl[top]         = rand() % 2;
	pattern1.slow_I_gl[bottom]      = rand() % 2;
    pattern1.fast_I_gl[top]         = rand() % 2;
    pattern1.fast_I_gl[bottom]      = rand() % 2;
    pattern1.slow_I_gladapt[top]    = rand() % 2;
    pattern1.slow_I_gladapt[bottom] = rand() % 2;
    pattern1.fast_I_gladapt[top]    = rand() % 2;
    pattern1.fast_I_gladapt[bottom] = rand() % 2;
    pattern1.slow_I_radapt[top]     = rand() % 2;
    pattern1.slow_I_radapt[bottom]  = rand() % 2;
    pattern1.fast_I_radapt[top]     = rand() % 2;
    pattern1.fast_I_radapt[bottom]  = rand() % 2;
    pattern1.bigcap[top]            = rand() % 2;
    pattern1.bigcap[bottom]         = rand() % 2;

	HICANN::set_neuron_config(this->h, pattern1);
	pattern2 = HICANN::get_neuron_config(this->h);

	EXPECT_GETTER_EQ(pattern1, pattern2);
}

TYPED_TEST(HICANNBackendTest, HorizontalRepeaterHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	std::array<HICANN::HorizontalRepeater, 64> pattern1, pattern2;
	HICANN::HorizontalRepeater rep = HICANN::HorizontalRepeater();

	srand (time(NULL));
	for(int i=0; i<64; i++){
		rep.setIdle();
		rep.setLen(rand() % 4);
		rep.setRen(rand() % 4);

		uint32_t mode = rand() % 5;
		std::bitset<2> dir = rand() % 3 + 1;

		if (mode==0) {
			if (dir[0]) rep.setOutput(left, dir[0]);
			if (dir[1]) rep.setOutput(right, dir[1]);
		}
		else if (mode==1) rep.setInput((dir[0]==false)?left:right);
		else if (mode==2) rep.setForwarding((dir[0]==false)?left:right);
		else if (mode==3) rep.setLoopback();
		else rep.setIdle();

		pattern1[i]=rep;
	}

	for(int i=0; i<64; i++){
		if (i%8!=6){ //set only non-sending repeaters
			HICANN::set_repeater(this->h, HLineOnHICANN(i).toHRepeaterOnHICANN(), pattern1[i]);
			pattern2[i]=HICANN::get_repeater(this->h, HLineOnHICANN(i).toHRepeaterOnHICANN());
		}
		else{
			pattern1[i]=HICANN::HorizontalRepeater();
			pattern2[i]=HICANN::HorizontalRepeater();
		}
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getRC(HCREP::REPEATER_CENTER_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_CENTER_RIGHT).print_config();
	// RET->getRC(HCREP::REPEATER_CENTER_LEFT).reset();
	// RET->getRC(HCREP::REPEATER_CENTER_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, VerticalRepeaterHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	std::array<HICANN::VerticalRepeater, 256> pattern1, pattern2;
	HICANN::VerticalRepeater rep = HICANN::VerticalRepeater();

	srand (time(NULL));
	for(int i=0; i<256; i++){
		rep.setIdle();
		rep.setLen(rand() % 4);
		rep.setRen(rand() % 4);

		uint32_t mode = rand() % 5;
		std::bitset<2> dir = rand() % 3 + 1;
		if (mode==0) {
			if (dir[0]) rep.setOutput(top, dir[0]);
			if (dir[1]) rep.setOutput(bottom, dir[1]);
		}
		else if (mode==1) rep.setInput((dir[0]==false)?top:bottom);
		else if (mode==2) rep.setForwarding((dir[0]==false)?top:bottom);
		else if (mode==3) rep.setLoopback();
		else rep.setIdle();

		pattern1[i]=rep;
	}

	for(int i=0; i<256; i++){
		HICANN::set_repeater(this->h, VLineOnHICANN(i).toVRepeaterOnHICANN(), pattern1[i]);
		pattern2[i]=HICANN::get_repeater(this->h, VLineOnHICANN(i).toVRepeaterOnHICANN());
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getRC(HCREP::REPEATER_TOP_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_TOP_RIGHT).print_config();
	//~ RET->getRC(HCREP::REPEATER_BOTTOM_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).print_config();
	// RET->getRC(HCREP::REPEATER_TOP_LEFT).reset();
	// RET->getRC(HCREP::REPEATER_TOP_RIGHT).reset();
	// RET->getRC(HCREP::REPEATER_BOTTOM_LEFT).reset();
	// RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, SendingRepeaterHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	std::array<HICANN::HorizontalRepeater, 8> pattern1, pattern2;
	HICANN::HorizontalRepeater rep = HICANN::HorizontalRepeater();

	srand (time(NULL));
	for(int i=0; i<8; i++){
		rep.setIdle();
		rep.setLen(rand() % 4);
		rep.setRen(rand() % 4);

		uint32_t mode = rand() % 5;
		std::bitset<2> dir = rand() % 3 + 1;
		if (mode==0) {
			if (dir[0]) rep.setOutput(left, dir[0]);
			if (dir[1]) rep.setOutput(right, dir[1]);
		}
		else if (mode==1) rep.setInput((dir[0]==false)?left:right);
		else if (mode==2) rep.setForwarding((dir[0]==false)?left:right);
		else if (mode==3) rep.setLoopback();
		else rep.setIdle();

		pattern1[i]=rep;
	}

	for(int i=0; i<8; i++){
		HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(i*8+6), left), pattern1[i]);
		pattern2[i]=HICANN::get_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(i*8+6), left));
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getRC(HCREP::REPEATER_CENTER_LEFT).print_config();
	// RET->getRC(HCREP::REPEATER_CENTER_LEFT).reset();
}

template<typename MergerCoordinate>
void randomize(HICANN::MergerTree& tree)
{
	for(auto merger_coord : halco::common::iter_all<MergerCoordinate>()) {
		HICANN::Merger m;
		m.config = rand() % 4;
		m.slow = rand() %2;
		tree[merger_coord] = m;
	}
}

template<typename MergerCoordinate>
void set_all(HICANN::MergerTree& tree, HICANN::Merger const& m)
{
	for(auto merger_coord : halco::common::iter_all<MergerCoordinate>()) {
		tree[merger_coord] = m;
	}
}

TYPED_TEST(HICANNBackendTest, MergerTreeHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	srand (time(NULL));
	for (size_t iter=0; iter<50; ++iter)
	{
		//generate random test data
		HICANN::MergerTree tree;

		randomize<Merger0OnHICANN>(tree);
		randomize<Merger1OnHICANN>(tree);
		randomize<Merger2OnHICANN>(tree);
		randomize<Merger3OnHICANN>(tree);

		HICANN::set_merger_tree(this->h, tree);
		HICANN::MergerTree ret = HICANN::get_merger_tree(this->h);

		EXPECT_GETTER_EQ(tree, ret);
	}

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getNC().print_config();
	// RET->getNC().nc_reset();
}

TYPED_TEST(HICANNBackendTest, DNCMergerHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	std::array<HICANN::DNCMergerLine, 8> pattern1, pattern2;
	HICANN::DNCMergerLine tree;
	HICANN::DNCMerger mer;

	srand (time(NULL));
	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mer.config = rand() % 4;
			mer.slow = rand() %2;
			tree[DNCMergerOnHICANN(j)] = mer;
		}

		for(int j=0; j<4; j++){ //2 neighbor loopbacks cannot be 1
			std::bitset<2> temp = rand() % 3;
			tree[DNCMergerOnHICANN(2*j)].loopback = temp[0];
			tree[DNCMergerOnHICANN(2*j+1)].loopback = temp[1];
		}

		pattern1[i] = tree;
	}

	for(int i=0; i<8; i++){ //8 times for safety :)
		HICANN::set_dnc_merger(this->h, pattern1[i]);
		pattern2[i]=HICANN::get_dnc_merger(this->h);
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getNC().print_config();
	// RET->getNC().nc_reset();
}

TYPED_TEST(HICANNBackendTest, PhaseHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	std::array<HICANN::Phase, 8> pattern1, pattern2;

	srand (time(NULL));
	for(int i=0; i<8; i++){
		pattern1[i] = rand() % 0x100;
	}

	for(int i=0; i<8; i++){ //8 times for safety :)
		HICANN::set_phase(this->h, pattern1[i]);
		pattern2[i]=HICANN::get_phase(this->h);
	}
	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getNC().print_config();
	// RET->getNC().nc_reset();
}

// Note: In random mode only address 0 is supported
TYPED_TEST(HICANNBackendTest, BackgroundGeneratorHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random test data
	HICANN::BackgroundGeneratorArray pattern1, pattern2;

	srand (time(NULL));
	for(int i=0; i<8; i++){
		HICANN::BackgroundGenerator bg;
		bool random = rand() % 2;
		HICANN::L1Address addr(0);
		if (!random)
			addr = HICANN::L1Address(rand() % 64);
		bg.enable(rand() % 2);
		bg.random(random);
		bg.seed(0xf0f0);
		bg.period(rand() % 0x10000);
		bg.address(addr);

		pattern1[i] = bg;
	}

	HICANN::set_background_generator(this->h, pattern1);
	pattern2=HICANN::get_background_generator(this->h);

	EXPECT_GETTER_EQ(pattern1, pattern2);
}

TYPED_TEST(HICANNBackendTest, AnalogOutputHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate some test data
	HICANN::Analog pattern1, pattern2;

	pattern1.set_fg_right(AnalogOnHICANN(1));
	pattern1.disable(AnalogOnHICANN(0));
	pattern1.enable(AnalogOnHICANN(1));

	HICANN::set_analog(this->h, pattern1);
	pattern2 = HICANN::get_analog(this->h);

	EXPECT_GETTER_EQ(pattern1, pattern2);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	// RET->getNBC().initzeros();
}

TYPED_TEST(HICANNBackendTest, StatusHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	HICANN::Status hs = HICANN::Status();
	FPGA::Status fs = FPGA::Status();

	hs = HICANN::get_hicann_status(this->h);
	fs = FPGA::get_fpga_status(this->f);

	EXPECT_GETTER_NO_THROW(hs.check());
	EXPECT_GETTER_NO_THROW(fs.check());

	//manual debugging help
	//~ cout << "HICANN status: \t0x" << hex << static_cast<uint32_t>(hs.getStatusReg())
	//~ << dec << ", CRC: " << static_cast<uint32_t>(hs.getCRCCount()) << endl;
	//~ cout << "DNC status: \t0x" << hex << static_cast<uint32_t>(ds.getStatusReg())
	//~ << dec << ", CRC: " << static_cast<uint32_t>(ds.getCRCCount()) << endl;
	//~ cout << "FPGA status: \t0x" << hex << static_cast<uint32_t>(fs.getStatusReg())
	//~ << dec << ", CRC: " << static_cast<uint32_t>(fs.getCRCCount()) << endl;
}

TYPED_TEST(HICANNBackendTest, StimCurrentHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random data
	srand(time(NULL));
	for (int i = 0; i < 4; i++) {
		HICANN::FGStimulus s;
		for (int j = 0; j < 129; j++)
			s[j] = rand() % 1024;

		HICANN::set_current_stimulus(this->h, FGBlockOnHICANN(Enum(i)), s);
		HICANN::FGStimulus r = HICANN::get_current_stimulus(this->h, FGBlockOnHICANN(Enum(i)));

		EXPECT_GETTER_EQ(s, r);
	}

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getFC(HCFG::FG_TOP_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_TOP_RIGHT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_RIGHT).print_config();
}

TYPED_TEST(HICANNBackendTest, FGSlownessHWTest) {
	HICANN::init(this->h, false);

	HICANN::FGControl ctrl;

	std::array<HICANN::FGConfig, 4> config;
	//generate random FG values
	srand(time(NULL));
	for (int i = 0; i < 4; i++){
		auto blk = FGBlockOnHICANN(Enum(i));
		HICANN::FGBlock& block = ctrl.getBlock(blk);
		for (int j = 0; j < 24; j++) {
			block.setSharedRaw(j, rand() % 1024);

			for (int k = 0; k < 128; k++) {
				block.setNeuronRaw(k, j, rand() % 1024);
			}
		}
		HICANN::FGConfig cfg;
		cfg.fg_bias          = rand() % 16;
		cfg.fg_biasn         = rand() % 16;
		cfg.pulselength      = rand() % 16;
		cfg.maxcycle         = rand() % 256;
		cfg.readtime         = rand() % 64;
		cfg.acceleratorstep  = rand() % 64;
		cfg.voltagewritetime = rand() % 64;
		cfg.currentwritetime = rand() % 64;
		config[i] = cfg;

		HICANN::set_fg_config(this->h, blk, config[i]);
	}

	size_t const max_time_in_ms = 1.2 * 2000; // ECM: +20%; see hicann-doc 4.4.7

	using namespace std::chrono;
	{
		// measure speed of 4 blocks
		auto start = system_clock::now();
		HICANN::set_fg_values(this->h, ctrl);
		auto end = system_clock::now();
		size_t allblocks_programming_time_in_ms = duration_cast<milliseconds>(end - start).count();
		EXPECT_LE(allblocks_programming_time_in_ms, max_time_in_ms);
	}

	{
		// measure speed of single (0th) block
		auto start = system_clock::now();
		auto blk = FGBlockOnHICANN(Enum(0));
		HICANN::set_fg_values(this->h, blk, ctrl.getBlock(blk));
		auto end = system_clock::now();
		size_t singleblock_programming_time_in_ms = duration_cast<milliseconds>(end - start).count();
		EXPECT_LE(singleblock_programming_time_in_ms, max_time_in_ms);
	}

	for (size_t i = 0; i < 4; i++) {
		FGBlockOnHICANN b {Enum{i}};
		HICANN::FGConfig cfg = HICANN::get_fg_config(this->h, b);
		EXPECT_EQ(cfg, config[i]);
	}

	// TODO: Alex, why this?
	ReticleControl * reticle = this->get_reticle(); // direct access for debugging
	RET->getFC(HCFG::FG_TOP_LEFT).reset();
	RET->getFC(HCFG::FG_TOP_RIGHT).reset();
	RET->getFC(HCFG::FG_BOTTOM_LEFT).reset();
	RET->getFC(HCFG::FG_BOTTOM_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, FGValuesHWTest) { //actual FG values not gettable (analog), only digital stuff
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	HICANN::FGControl ctrl;
	std::array<HICANN::FGConfig, 4> config;

	//generate random data
	srand(time(NULL));
	for (int i = 0; i < 4; i++){
		HICANN::FGBlock& block = ctrl.getBlock(FGBlockOnHICANN(Enum(i)));
		for (int j = 0; j < 24; j++) {
			block.setSharedRaw(j, rand() % 1024);

			for (int k = 0; k < 128; k++) {
				block.setNeuronRaw(k, j, rand() % 1024);
			}
		}
		HICANN::FGConfig cfg;
		cfg.fg_bias          = rand() % 16;
		cfg.fg_biasn         = rand() % 16;
		cfg.pulselength      = rand() % 16;
		cfg.maxcycle         = rand() % 256;
		cfg.readtime         = rand() % 64;
		cfg.acceleratorstep  = rand() % 64;
		cfg.voltagewritetime = rand() % 64;
		cfg.currentwritetime = rand() % 64;
		config[i] = cfg;

		auto blk = FGBlockOnHICANN(Enum(i));
		HICANN::set_fg_config(this->h, blk, config[i]);
	}


	HICANN::set_fg_values(this->h, ctrl);

	for (size_t i = 0; i < 4; i++)
	{
		FGBlockOnHICANN b {Enum{i}};
		HICANN::FGConfig cfg = HICANN::get_fg_config(this->h, b);
		EXPECT_GETTER_EQ(cfg, config[i]);
	}

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getFC(HCFG::FG_TOP_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_TOP_RIGHT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_RIGHT).print_config();
	// RET->getFC(HCFG::FG_TOP_LEFT).reset();
	// RET->getFC(HCFG::FG_TOP_RIGHT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_LEFT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_RIGHT).reset();
}

TYPED_TEST(HICANNBackendTest, FGRealValuesHWTest) { //actual FG values not gettable (analog), only digital stuff
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate random data
	srand(time(NULL));

	HICANN::FGControl fgc;

	//FG parameters for neurons just under the threshold (almost spiking)
	for (size_t neuron_number = 0; neuron_number < 512; neuron_number++)
	{
		NeuronOnHICANN nrn{Enum(neuron_number)};

		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, rand() % 256 + 256);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, rand() % 256 );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, rand() % 256 + 512 );
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_bexp, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convi, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_convx, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 0);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, rand() % 512);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbi, 800);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbx, 800);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 100);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 511);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 300);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::I_spikeamp, 1023);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 500);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syni, 350);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 850);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 850);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_synx, 568);
		fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, rand() % 256 + 300); //threshold neuron0
	}

	for (size_t i = 0; i < 4; i++){
		FGBlockOnHICANN block {Enum{i}};
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
		if (i == 0 || i == 2) {
			fgc.setShared(block, HICANN::shared_parameter::V_bout, 1023);
			fgc.setShared(block, HICANN::shared_parameter::V_clra, 0);
		} else {
			fgc.setShared(block, HICANN::shared_parameter::V_bexp, 1023);
			fgc.setShared(block, HICANN::shared_parameter::V_clrc, 0);
		}
	}

	//HICANN::set_PLL_frequency(this->h, 100);
	for (size_t i = 0; i < 4; i++){
		FGBlockOnHICANN b{Enum{i}};
		HICANN::set_fg_config(this->h, b, HICANN::FGConfig());
		HICANN::set_fg_values(this->h, b, fgc.getBlock(b));
	}
	//HICANN::set_PLL_frequency(this->h, 150);

	// ReticleControl& reticle = *this->h.this->get_reticle(); // direct access for debugging
	//~ RET->getFC(HCFG::FG_TOP_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_TOP_RIGHT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_RIGHT).print_config();
	// RET->getFC(HCFG::FG_TOP_LEFT).reset();
	// RET->getFC(HCFG::FG_TOP_RIGHT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_LEFT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_RIGHT).reset();
}

/**
 * Following tests are for testing the correctness of HMFBackend to the hardware.
 * The above tests are not sufficient as they only utilize HMFBackend functionality
 * in both write- and read- directions.
 * The tests below use HMFBackend to write and low-level functions to read. This way
 * one can distinguish errors that would balance each other out in HMFBackend.
 */
TYPED_TEST(HICANNBackendTest, WriteSparseMatricesHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	///test crossbars
	std::array<bool, 4> pattern {{ true, false, false, false }};

	ci_addr_t addr;
	ci_data_t data;

	HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(0), left, pattern);
	HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(0), right, pattern);

	if (auto * reticle = this->get_reticle()) { // Only when testing on hardware
		RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).read_cfg(63);
		RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(1, data);
		RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).read_cfg(63);
		RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(8, data);
	}

	pattern = {{ false, true, false, false }};

	HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(2), left, pattern);
	HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(3), right, pattern);

	if (auto * reticle = this->get_reticle()) { // Only when testing on hardware
		RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).read_cfg(61);
		RET->getLC(HCL1::L1SWITCH_CENTER_LEFT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(2, data);
		RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).read_cfg(60);
		RET->getLC(HCL1::L1SWITCH_CENTER_RIGHT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(4, data);
	}

	///test synapse switches
	HICANN::SynapseSwitchRow dd {{{ false, false, false, false, true, false, false, false,
									false, false, false, false, false, false, false, false }}};

	HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(10), left), dd);
	HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(120), left), dd);
	HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(20), right), dd);
	HICANN::set_syndriver_switch_row(this->h, SynapseSwitchRowOnHICANN(Y(140), right), dd);

	if (auto * reticle = this->get_reticle()) {
		RET->getLC(HCL1::L1SWITCH_TOP_LEFT).read_cfg(101);
		RET->getLC(HCL1::L1SWITCH_TOP_LEFT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(0x800, data);
		RET->getLC(HCL1::L1SWITCH_BOTTOM_LEFT).read_cfg(8);
		RET->getLC(HCL1::L1SWITCH_BOTTOM_LEFT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(0x800, data);
		RET->getLC(HCL1::L1SWITCH_TOP_RIGHT).read_cfg(91);
		RET->getLC(HCL1::L1SWITCH_TOP_RIGHT).get_read_cfg(addr, data);
		EXPECT_GETTER_EQ(0x10, data);
		RET->getLC(HCL1::L1SWITCH_BOTTOM_RIGHT).read_cfg(28);
		RET->getLC(HCL1::L1SWITCH_BOTTOM_RIGHT).get_read_cfg(addr, data);
	}
}

TYPED_TEST(HICANNBackendTest, WriteSynapsesHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	///test synapse weights
	HICANN::WeightRow row, row0;
	std::vector <uint32_t> data(32,0);
	std::generate(row.begin(), row.end(), IncrementingSequence<HICANN::SynapseWeight>(0xf));

	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(3), left), top), row);    //top line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(3), left), bottom), row0);    //bottom line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(116), left), top), row0); //top line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(116), left), bottom), row);   //bottom line

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getSC(HCSYN::SYNAPSE_TOP).read_row(217, data, false);
		EXPECT_GETTER_EQ(0x76543210, data[0]);
		RET->getSC(HCSYN::SYNAPSE_TOP).read_row(216, data, false);
		EXPECT_GETTER_EQ(0, data[0]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_row(8, data, false);
		EXPECT_GETTER_EQ(0, data[0]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_row(9, data, false);
		EXPECT_GETTER_EQ(0x76543210, data[0]);
	}

	std::rotate(row.begin(), row.begin()+1, row.end());
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(4), right), top), row0);  //top line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(4), right), bottom), row);    //bottom line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(115), right), top), row); //top line
	HICANN::set_weights_row(this->h, SynapseRowOnHICANN(SynapseDriverOnHICANN(Y(115), right), bottom), row0); //bottom line

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getSC(HCSYN::SYNAPSE_TOP).read_row(215, data, false);
		EXPECT_GETTER_EQ(0, data[1]);
		RET->getSC(HCSYN::SYNAPSE_TOP).read_row(214, data, false);
		EXPECT_GETTER_EQ(0x0fedcba9, data[1]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_row(6, data, false);
		EXPECT_GETTER_EQ(0x0fedcba9, data[1]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_row(7, data, false);
		EXPECT_GETTER_EQ(0, data[1]);
	}

	///test synapse decoders
	HICANN::DecoderDoubleRow drow;
	vector <uint32_t> data_bot(32,0);
	vector <uint32_t> data_top(32,0);
	std::generate(drow[0].begin(), drow[0].end(), IncrementingSequence<HICANN::SynapseDecoder>(0xf));
	std::fill(drow[1].begin(), drow[1].end(), HICANN::SynapseDecoder(0));

	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(5), left), drow);
	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(118), left), drow);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getSC(HCSYN::SYNAPSE_TOP).read_decoder(212, data_bot, data_top);
		EXPECT_GETTER_EQ(0x76543210, data_top[0]);
		EXPECT_GETTER_EQ(0, data_bot[0]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_decoder(12, data_bot, data_top);
		EXPECT_GETTER_EQ(0xfedcba98, data_bot[1]);
		EXPECT_GETTER_EQ(0, data_top[1]);
	}

	std::rotate(drow[0].begin(), drow[0].begin()+2, drow[0].end());
	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(6), right), drow);
	HICANN::set_decoder_double_row(this->h, SynapseDriverOnHICANN(Y(117), right), drow);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getSC(HCSYN::SYNAPSE_TOP).read_decoder(210, data_bot, data_top);
		EXPECT_GETTER_EQ(0x98765432, data_top[0]);
		EXPECT_GETTER_EQ(0, data_bot[0]);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_decoder(10, data_bot, data_top);
		EXPECT_GETTER_EQ(0x10fedcba, data_bot[1]);
		EXPECT_GETTER_EQ(0, data_top[1]);
	}

	//~ RET->getSC(HCSYN::SYNAPSE_TOP).print_weight();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).print_weight();
	//~ RET->getSC(HCSYN::SYNAPSE_TOP).print_decoder();
	//~ RET->getSC(HCSYN::SYNAPSE_BOTTOM).print_decoder();
}

TYPED_TEST(HICANNBackendTest, WriteSynapseDriverHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	// RET->getSC(HCSYN::SYNAPSE_TOP).reset_drivers();
	// RET->getSC(HCSYN::SYNAPSE_BOTTOM).reset_drivers();

	uint32_t cfgtop, pdrvtop, gmaxtop, cfgbot, pdrvbot, gmaxbot;

	//top/bottom here refers to top/bottom lines INSIDE the row, not parts of ANNCORE
	HICANN::SynapseDriver driver;
	driver[bottom].set_decoder(top, HICANN::DriverDecoder(0));
	driver[top].set_decoder(top, HICANN::DriverDecoder(1));
	driver[bottom].set_decoder(bottom, HICANN::DriverDecoder(2));
	driver[top].set_decoder(bottom, HICANN::DriverDecoder(3));
	driver[top].set_gmax_div(left, 9);
	driver[top].set_gmax_div(right, 13);
	driver[bottom].set_gmax_div(left, 7);
	driver[bottom].set_gmax_div(right, 10);
	driver[top].set_gmax(3);
	driver[bottom].set_gmax(2);
	driver[top].set_syn_in(left, 0);
	driver[top].set_syn_in(right, 1);
	driver[bottom].set_syn_in(left, 1);
	driver[bottom].set_syn_in(right, 0);
	driver.stp_cap              = 5;
	driver.enable               = 1;
	driver.locin                = 1;
	driver.connect_neighbor     = 0;
	driver.stp_enable           = 0;
	driver.stp_mode             = 1;

	HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Y(25), geometry::left), driver);
	HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Y(121), geometry::right), driver);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getSC(HCSYN::SYNAPSE_TOP).read_driver(172, cfgbot, pdrvbot, gmaxbot, cfgtop, pdrvtop, gmaxtop);
		EXPECT_GETTER_EQ(0x69, cfgbot); EXPECT_GETTER_EQ(0xde, cfgtop); EXPECT_GETTER_EQ(0xa7, gmaxbot);
		EXPECT_GETTER_EQ(0xd9, gmaxtop); EXPECT_GETTER_EQ(0x40, pdrvbot); EXPECT_GETTER_EQ(0xa8, pdrvtop);
		RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_driver(18, cfgbot, pdrvbot, gmaxbot, cfgtop, pdrvtop, gmaxtop);
		EXPECT_GETTER_EQ(0x6e, cfgbot); EXPECT_GETTER_EQ(0xd9, cfgtop); EXPECT_GETTER_EQ(0xd9, gmaxbot);
		EXPECT_GETTER_EQ(0xa7, gmaxtop); EXPECT_GETTER_EQ(0x51, pdrvbot); EXPECT_GETTER_EQ(0x20, pdrvtop);
	}
}

TYPED_TEST(HICANNBackendTest, WriteNeuronBuilderHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	// RET->getNBC().initzeros();

	static std::array<int, NeuronOnQuad::enum_type::end> const map =
		{{ 1, 3, 0, 2, }};

	size_t const num_quads = 20;
	for (int ii=0; ii<128; ii+=128/num_quads)
	{
		size_t const address_offset = 4*ii;


		///test denmem quad
		HICANN::NeuronQuad nquad = HICANN::NeuronQuad();

		HICANN::Neuron& neu0 = nquad[NeuronOnQuad(X(0), Y(1))];
		neu0.address(HICANN::L1Address(21));
		neu0.activate_firing(true);
		neu0.enable_fire_input(false);
		neu0.enable_aout(false);
		neu0.enable_current_input(false);

		HICANN::Neuron& neu1 = nquad[NeuronOnQuad(X(0), Y(0))];
		neu1.address(HICANN::L1Address(42));
		neu1.activate_firing(false);
		neu1.enable_fire_input(true);
		neu1.enable_aout(true);
		neu1.enable_current_input(false);

		HICANN::Neuron& neu2 = nquad[NeuronOnQuad(X(1), Y(1))];
		neu2.address(HICANN::L1Address(0));
		neu2.activate_firing(false);
		neu2.enable_fire_input(false);
		neu2.enable_aout(true);
		neu2.enable_current_input(true);

		HICANN::Neuron& neu3 = nquad[NeuronOnQuad(X(1), Y(0))];
		neu3.address(HICANN::L1Address(15));
		neu3.activate_firing(true);
		neu3.enable_fire_input(true);
		neu3.enable_aout(false);
		neu3.enable_current_input(false);

		nquad.setVerticalInterconnect(X(0), true);
		nquad.setVerticalInterconnect(X(1), false);
		nquad.setHorizontalInterconnect(Y(0), true);
		nquad.setHorizontalInterconnect(Y(1), false);


		HICANN::set_denmem_quad(this->h, QuadOnHICANN(ii), nquad);

		if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
			for (int jj=0; jj<4; jj++)
			{
				std::array<ci_data_t, 3> data;

				size_t iter = 100;
				// reading is unreliable, see:  https://brainscales-r.kip.uni-heidelberg.de/issues/818 .
				// Therefore, read mutiple times and continue, only if equal.
				do {
					for (auto& entry : data)
					{
						ci_addr_t addr;

						RET->getNBC().read_data(address_offset + map[jj]);
						RET->getNBC().get_read_data(addr, entry);

						ASSERT_GETTER_EQ(address_offset + map[jj], addr);
					}

					--iter;
				} while (!std::equal(data.begin(), data.end(), data.begin()) && iter);
				ASSERT_GETTER_TRUE(std::equal(data.begin(), data.end(), data.begin()));

				ASSERT_GETTER_EQ(data[0], std::bitset<25>(data[0]).to_ulong());

				DenmemConfig config(jj, data[0]);
				verify(nquad, config, false);
			}
		}
	}
}

TYPED_TEST(HICANNBackendTest, WriteNeuronConfigHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	ci_data_t data;
	ci_addr_t addr;

	//~ RET->getNBC().print_config();
	HICANN::NeuronConfig pattern1 = HICANN::NeuronConfig();

	pattern1.slow_I_gl[top]         = 1;
	pattern1.slow_I_gl[bottom]      = 0;
    pattern1.fast_I_gl[top]         = 0;
    pattern1.fast_I_gl[bottom]      = 1;
    pattern1.slow_I_gladapt[top]    = 1;
    pattern1.slow_I_gladapt[bottom] = 1;
    pattern1.fast_I_gladapt[top]    = 0;
    pattern1.fast_I_gladapt[bottom] = 1;
    pattern1.slow_I_radapt[top]     = 1;
    pattern1.slow_I_radapt[bottom]  = 0;
    pattern1.fast_I_radapt[top]     = 0;
    pattern1.fast_I_radapt[bottom]  = 0;
    pattern1.bigcap[top]            = 1;
    pattern1.bigcap[bottom]         = 0;

	HICANN::set_neuron_config(this->h, pattern1);
	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getNBC().read_data(NeuronBuilderControl::NREGBASE);
		RET->getNBC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x1ee86, data);
	}
}

TYPED_TEST(HICANNBackendTest, WriteRepeatersHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	///testing hotizontal repeaters
	HICANN::HorizontalRepeater hrep = HICANN::HorizontalRepeater();
	hrep.setLen(0);
	hrep.setRen(2);

	hrep.setInput(right);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(3), right), hrep);
	hrep.setInput(left);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(5), right), hrep);
	hrep.setOutput(right);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(15), right), hrep);
	hrep.setOutput(left);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(17), right), hrep);
	hrep.setForwarding(right);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(19), right), hrep);
	hrep.setLoopback();
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(21), right), hrep);
	hrep.setLen(2);
	hrep.setForwarding(left);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(28), left), hrep);
	hrep.setForwarding(right);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(32), left), hrep);
	hrep.setIdle();
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(34), left), hrep);
	hrep.setInput(right);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(36), left), hrep);
	hrep.setOutput(left);
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(40), left), hrep);
	hrep.setLoopback();
	HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(42), left), hrep);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		uint32_t data;
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(2); //horiz. line 3
		EXPECT_GETTER_EQ(0xa8, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(3); //horiz. line 5
		EXPECT_GETTER_EQ(0x98, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(8); //horiz. line 15
		EXPECT_GETTER_EQ(0x68, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(9); //horiz. line 17
		EXPECT_GETTER_EQ(0x78, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(10); //horiz. line 19
		EXPECT_GETTER_EQ(0x28, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_RIGHT).read_data(11); //horiz. line 21
		EXPECT_GETTER_EQ(0xc8, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(17); //horiz. line 28
		EXPECT_GETTER_EQ(0x2a, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(15); //horiz. line 32
		EXPECT_GETTER_EQ(0x1a, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(14); //horiz. line 34
		EXPECT_GETTER_EQ(0x0a, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(13); //horiz. line 36
		EXPECT_GETTER_EQ(0x9a, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(11); //horiz. line 40
		EXPECT_GETTER_EQ(0x6a, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(10); //horiz. line 42
		EXPECT_GETTER_EQ(0xca, data);
	}

	///testing sending repeaters
	HICANN::HorizontalRepeater srep = HICANN::HorizontalRepeater();
	srep.setLen(3);
	srep.setRen(1);

	srep.setForwarding(right);
	HICANN::set_repeater(this->h, SendingRepeaterOnHICANN(6).toHRepeaterOnHICANN(), srep); //sending line 54
	srep.setOutput(right);
	srep.setOutput(left);
	HICANN::set_repeater(this->h, SendingRepeaterOnHICANN(0).toHRepeaterOnHICANN(), srep); //sending line 6
	srep.setOutput(left, false);
	HICANN::set_repeater(this->h, SendingRepeaterOnHICANN(2).toHRepeaterOnHICANN(), srep); //sending line 22
	srep.setInput(right);
	HICANN::set_repeater(this->h, SendingRepeaterOnHICANN(1).toHRepeaterOnHICANN(), srep); //sending line 14
	srep.setLoopback();
	HICANN::set_repeater(this->h, SendingRepeaterOnHICANN(3).toHRepeaterOnHICANN(), srep); //sending line 30

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		uint32_t data;
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(0); //sending line 62, nothing was written here
		EXPECT_GETTER_EQ(0, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(4); //sending line 54
		EXPECT_GETTER_EQ(0x17, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(28); //sending line 6
		EXPECT_GETTER_EQ(0x77, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(20); //sending line 22
		EXPECT_GETTER_EQ(0x57, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(24); //sending line 14
		EXPECT_GETTER_EQ(0x97, data);
		data = RET->getRC(HCREP::REPEATER_CENTER_LEFT).read_data(16); //sending line 30
		EXPECT_GETTER_EQ(0xc7, data);
	}

	///testing vertical repeaters
	HICANN::VerticalRepeater vrep = HICANN::VerticalRepeater();
	vrep.setLen(1);
	vrep.setRen(2);

	vrep.setInput(top);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(35), top), vrep);
	vrep.setOutput(bottom);
	vrep.setOutput(top);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(33), top), vrep);
	vrep.setOutput(bottom, false);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(36), bottom), vrep);
	vrep.setForwarding(bottom);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(34), bottom), vrep);
	vrep.setForwarding(top);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(160), top), vrep);
	vrep.setForwarding(bottom);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(162), top), vrep);
	vrep.setInput(top);
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(161), bottom), vrep);
	vrep.setLoopback();
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(165), bottom), vrep);
	vrep.setIdle();
	HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(163), bottom), vrep);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		uint32_t data;
		data = RET->getRC(HCREP::REPEATER_TOP_LEFT).read_data(0); //vert. line 1, nothing was written here
		EXPECT_GETTER_EQ(0, data);
		data = RET->getRC(HCREP::REPEATER_TOP_LEFT).read_data(18); //vert. line 35
		EXPECT_GETTER_EQ(0xa9, data);
		data = RET->getRC(HCREP::REPEATER_TOP_LEFT).read_data(17); //vert. line 33
		EXPECT_GETTER_EQ(0x79, data);
		data = RET->getRC(HCREP::REPEATER_BOTTOM_LEFT).read_data(18); //vert. line 36
		EXPECT_GETTER_EQ(0x59, data);
		data = RET->getRC(HCREP::REPEATER_BOTTOM_LEFT).read_data(17); //vert. line 34
		EXPECT_GETTER_EQ(0x29, data);
		data = RET->getRC(HCREP::REPEATER_TOP_RIGHT).read_data(48); //vert. line 160
		EXPECT_GETTER_EQ(0x29, data);
		data = RET->getRC(HCREP::REPEATER_TOP_RIGHT).read_data(47); //vert. line 162
		EXPECT_GETTER_EQ(0x19, data);
		data = RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).read_data(47); //vert. line 161
		EXPECT_GETTER_EQ(0x99, data);
		data = RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).read_data(45); //vert. line 165
		EXPECT_GETTER_EQ(0xc9, data);
		data = RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).read_data(46); //vert. line 163
		EXPECT_GETTER_EQ(0x9, data);
	}

	//~ RET->getRC(HCREP::REPEATER_TOP_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_TOP_RIGHT).print_config();
	//~ RET->getRC(HCREP::REPEATER_CENTER_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_CENTER_RIGHT).print_config();
	//~ RET->getRC(HCREP::REPEATER_BOTTOM_LEFT).print_config();
	//~ RET->getRC(HCREP::REPEATER_BOTTOM_RIGHT).print_config();
}

TYPED_TEST(HICANNBackendTest, WriteNeuronControlHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	// RET->getNC().nc_reset();

	///testing mergers
	HICANN::MergerTree tree;
	ci_addr_t addr;
	ci_data_t data;

	HICANN::Merger merger(HICANN::Merger::MERGE);
	set_all<Merger0OnHICANN>(tree, merger);
	set_all<Merger1OnHICANN>(tree, merger);
	set_all<Merger2OnHICANN>(tree, merger);
	set_all<Merger3OnHICANN>(tree, merger);

	tree[Merger0OnHICANN(0)].config = HICANN::Merger::LEFT_ONLY;
	tree[Merger0OnHICANN(1)].config = HICANN::Merger::RIGHT_ONLY;
	tree[Merger0OnHICANN(7)].config = HICANN::Merger::LEFT_ONLY;
	tree[Merger1OnHICANN(1)].config = HICANN::Merger::RIGHT_ONLY;
	tree[Merger2OnHICANN(1)].config = HICANN::Merger::LEFT_ONLY;
	tree[Merger3OnHICANN(0)].config = HICANN::Merger::RIGHT_ONLY;

	tree[Merger0OnHICANN(0)].slow   = true;
	tree[Merger0OnHICANN(4)].slow   = true;
	tree[Merger0OnHICANN(5)].slow   = true;
	tree[Merger1OnHICANN(3)].slow   = true;
	tree[Merger2OnHICANN(0)].slow   = true;
	tree[Merger3OnHICANN(0)].slow   = true;

	HICANN::set_merger_tree(this->h, tree);
	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getNC().read_data(NeuronControl::nc_enable);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x2b3e, data);
		RET->getNC().read_data(NeuronControl::nc_select);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x1081, data);
		RET->getNC().read_data(NeuronControl::nc_slow);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x618c, data);
	}

	///testing DNC mergers
	HICANN::DNCMergerLine dtree = HICANN::DNCMergerLine();
	dtree[Coordinate::DNCMergerOnHICANN{0}].config = HICANN::Merger::LEFT_ONLY;
	dtree[Coordinate::DNCMergerOnHICANN{1}].config = HICANN::Merger::LEFT_ONLY;
	dtree[Coordinate::DNCMergerOnHICANN{2}].config = HICANN::Merger::RIGHT_ONLY;
	dtree[Coordinate::DNCMergerOnHICANN{5}].config = HICANN::Merger::RIGHT_ONLY;
	dtree[Coordinate::DNCMergerOnHICANN{0}].slow   = true;
	dtree[Coordinate::DNCMergerOnHICANN{6}].slow   = true;
	dtree[Coordinate::DNCMergerOnHICANN{7}].slow   = true;
	dtree[Coordinate::DNCMergerOnHICANN{0}].loopback = true;
	dtree[Coordinate::DNCMergerOnHICANN{5}].loopback = true;

	HICANN::set_dnc_merger(this->h, dtree);
	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getNC().read_data(NeuronControl::nc_dncmerger);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x831b, data);
		RET->getNC().read_data(NeuronControl::nc_dncloopb);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0xc048, data);
	}

	///testing phase
	HICANN::Phase phase = 0;
	phase[1] = 1;
	phase[3] = 1;
	phase[7] = 1;

	HICANN::set_phase(this->h, phase);
	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getNC().read_data(NeuronControl::nc_phase);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0x51, data);
	}

	///testing background generators
	HICANN::BackgroundGeneratorArray bgarray;
	HICANN::BackgroundGenerator bg = HICANN::BackgroundGenerator();

	for(int i=0; i<8; i++){
		bg.enable(i%2);
		bg.random(!(i%2));
		bg.seed(500);
		bg.period(10*i);
		bg.address(HICANN::L1Address(i));

		bgarray[i] = bg;
	}

	HICANN::set_background_generator(this->h, bgarray);
	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getNC().read_data(NeuronControl::nc_seed);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(500, data);

		RET->getNC().read_data(NeuronControl::nc_randomreset);
		RET->getNC().get_read_data(addr, data);
		EXPECT_GETTER_EQ(0xaa55, data);

		for (int i=0; i<8; i++){
			RET->getNC().read_data(NeuronControl::nc_period+i);
			RET->getNC().get_read_data(addr, data);
			EXPECT_GETTER_EQ(10*(7-i), data); //BEGs swapped
		}

		for (int i=0; i<4; i++){
			RET->getNC().read_data(NeuronControl::nc_nnumber+i);
			RET->getNC().get_read_data(addr, data);
			int j = 7 - 2 * i;
			EXPECT_GETTER_EQ((j-1)<<8|j, data);
		}
	}

	//~ RET->getNC().print_config();
}

TYPED_TEST(HICANNBackendTest, WriteFloatingGateHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	// RET->getFC(HCFG::FG_TOP_LEFT).reset();
	// RET->getFC(HCFG::FG_TOP_RIGHT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_LEFT).reset();
	// RET->getFC(HCFG::FG_BOTTOM_RIGHT).reset();

	///test ram banks
	HICANN::FGStimulus stim;
	ci_addr_t addr;
	ci_data_t data;
	uint32_t biasreg, opreg;

	for (size_t i = 0; i < stim.size(); i++)
		stim[i] = i;
	stim.setContinuous(false);
	HICANN::set_current_stimulus(this->h, FGBlockOnHICANN(Enum(0)), stim);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		for (size_t i = 0; i < 64; i++) {
			RET->getFC(HCFG::FG_TOP_LEFT).read_data(i);
			RET->getFC(HCFG::FG_TOP_LEFT).get_read_data(addr, data);
			EXPECT_GETTER_EQ(((2*i+1)<<10|2*i), data);
		}
	}

	///test configuration
	HICANN::FGBlock block = HICANN::FGBlock();

	HICANN::FGConfig config;
	config.fg_bias          = 5;
	config.fg_biasn         = 3;
	config.pulselength      = 7;
	config.maxcycle         = 100;
	config.readtime         = 30;
	config.acceleratorstep  = 21;
	config.voltagewritetime = 42;
	config.currentwritetime = 25;
	HICANN::set_fg_config(this->h, FGBlockOnHICANN(Enum(3)), config);
	HICANN::set_fg_values(this->h, FGBlockOnHICANN(Enum(3)), block);

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		RET->getFC(HCFG::FG_BOTTOM_RIGHT).read_data(facets::FGControl::REG_BIAS);
		RET->getFC(HCFG::FG_BOTTOM_RIGHT).get_read_data(addr, biasreg);
		RET->getFC(HCFG::FG_BOTTOM_RIGHT).read_data(facets::FGControl::REG_OP);
		RET->getFC(HCFG::FG_BOTTOM_RIGHT).get_read_data(addr, opreg);
		EXPECT_GETTER_EQ(0x7ca, biasreg);
		EXPECT_GETTER_EQ(0x66a55e64, opreg);
	}

	//~ RET->getFC(HCFG::FG_TOP_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_TOP_RIGHT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_LEFT).print_config();
	//~ RET->getFC(HCFG::FG_BOTTOM_RIGHT).print_config();
}

TYPED_TEST(HICANNBackendTest, WriteSTDPConfigHWTest) {
	HICANN::init(this->h, false); //initialize HICANN to be able to do the test in the first place

	//generate test data
	HICANN::STDPControl pattern;

	pattern.eval = HICANN::STDPEval();
	pattern.lut.set_defaults();
	pattern.lut.causal[7] = 15;
	pattern.without_reset = true;
	pattern.read_causal = true;
	pattern.read_acausal = false;
	pattern.continuous_autoupdate = false;
	pattern.set_first_row(2);
	pattern.set_last_row(8);
	pattern.timing.wrdel = 2;
	pattern.timing.outdel = 3;
	pattern.timing.predel = 4;
	HICANN::set_stdp_config(this->h, top, pattern);

	pattern.without_reset = false;
	pattern.read_acausal = true;
	pattern.continuous_autoupdate = true;
	pattern.set_first_row(10);
	pattern.set_last_row(223);
	HICANN::set_stdp_config(this->h, bottom, pattern);

	std::array<uint32_t, 9> hwdata;

	if (auto * reticle = this->get_reticle()) { // Test only for a real hardware test
		hwdata[0] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut);
		hwdata[1] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut+1);
		hwdata[2] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut+2);
		hwdata[3] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut+3);
		hwdata[4] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut+4);
		hwdata[5] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_lut+5);
		hwdata[6] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_ctrlreg);
		hwdata[7] = RET->getSC(HCSYN::SYNAPSE_TOP).read_data(facets::SynapseControl::sc_cnfgreg);

		EXPECT_GETTER_EQ(0x0ecd89ab, hwdata[0]);
		EXPECT_GETTER_EQ(0x01234567, hwdata[1]);
		EXPECT_GETTER_EQ(0x89abcdef, hwdata[2]);
		EXPECT_GETTER_EQ(0x456723ff, hwdata[3]);
		EXPECT_GETTER_EQ(0x01234567, hwdata[4]);
		EXPECT_GETTER_EQ(0x89abcdef, hwdata[5]);
		EXPECT_GETTER_EQ(0x58080250, hwdata[6]);
		EXPECT_GETTER_EQ(0x08ff4f09, hwdata[7]);

		hwdata[8] = RET->getSC(HCSYN::SYNAPSE_BOTTOM).read_data(facets::SynapseControl::sc_ctrlreg);
		EXPECT_GETTER_EQ(0x70df0a70, hwdata[8]);
	}
}

/** End of Write-tests */

TYPED_TEST(HICANNBackendTest, DISABLED_HICANNBEGtoSyndriverHWTest) {
	// BG Generator 7 ->  analog out of Synapse Driver
	HICANN::init(this->h, false);

	// set PLL frequency
	//HICANN::set_PLL_frequency(this->h,150);

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

	uint16_t period = 150;

	bg.enable(true);
	bg.random(false);
	bg.seed(200);
	bg.period(period);
	bg.address(HICANN::L1Address(0));
	bgarray[7] = bg;
	HICANN::set_background_generator(this->h, bgarray);

	// Enable HICANN::Repeater
	HICANN::set_repeater_block( this->h, RepeaterBlockOnHICANN(X(0), Y(1)), HICANN::RepeaterBlock());

	HICANN::HorizontalRepeater sr;
	// enable output on this hicann
	sr.setOutput(right);
	HICANN::set_repeater( this->h, HRepeaterOnHICANN(HLineOnHICANN(6), left), sr);

	// Crossbar Switch
	HICANN::Crossbar cb;
	cb.set(VLineOnHICANN(28), HLineOnHICANN(6), true);
	HICANN::CrossbarRow row_cfg =  cb.get_row(HLineOnHICANN(6), left);
	EXPECT_GETTER_EQ(true, row_cfg[0]);
	HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6), left, row_cfg);

	bool check_repeater = true;
	if (check_repeater) {
		// readout on bottom repeater
		HICANN::VerticalRepeater vr;
		HICANN::RepeaterBlock rb = HICANN::RepeaterBlock();
		//configure receiving repeater
		vr.setInput(bottom);
		HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(28), bottom), vr);

		usleep(500); //time for the dll to lock

		TestPortOnRepeaterBlock tp(0);
		RepeaterBlockOnHICANN block(X(0), Y(2));

		//configure receiving repeater block
		rb.start_tdi[tp] = false; //reset the full flag
		HICANN::set_repeater_block(this->h, block, rb);
		rb.start_tdi[tp] = true; //start recording received data
		HICANN::set_repeater_block(this->h, block, rb);

		usleep(1000);  //recording lasts for ca. 4 µs - 1ms

		rb = HICANN::get_repeater_block(this->h, RepeaterBlockOnHICANN(block));
		std::array<HICANN::RepeaterBlock::TestEvent, 3> test = rb.tdi_data[tp];

		size_t success_counter = 0;
		for (size_t x = 0; x < 2; x++) //see how many events come with correct delay
			if((std::abs( std::abs(test[x+1].time - test[x].time) - period )) < 3)
				success_counter++;

		cout << std::dec << "From repeater " << 28 << " received: ";
		if (rb.full_flag[tp]) cout << "Full flag is set" << endl;
		else cout << "Full flag is NOT set" << endl;
		cout << "HICANN::Neuron number " << (uint32_t)test[0].address << " at time " << (uint32_t)test[0].time << endl;
		cout << "HICANN::Neuron number " << (uint32_t)test[1].address << " at time " << (uint32_t)test[1].time << endl;
		cout << "HICANN::Neuron number " << (uint32_t)test[2].address << " at time " << (uint32_t)test[2].time << endl << endl;

		rb.start_tdi[tp] = false; //reset the full flag TWO (!) times
		HICANN::set_repeater_block(this->h, RepeaterBlockOnHICANN(block), rb);
		HICANN::set_repeater_block(this->h, RepeaterBlockOnHICANN(block), rb);

		//set repeater mode to IDLE to prevent conflicts
		vr.setIdle();
		HICANN::set_repeater(this->h, VRepeaterOnHICANN(VLineOnHICANN(28), bottom), vr);

		EXPECT_GETTER_GT(success_counter, 1);
	}

	// Syndriver Switch
	HICANN::SynapseSwitch  sw;
	SynapseSwitchRowOnHICANN addr(Y(111), left);
	sw.set(VLineOnHICANN(28), addr.line(), true);
	HICANN::SynapseSwitchRow srow_cfg = sw.get_row(addr);
	EXPECT_GETTER_EQ(true, srow_cfg[0]);
	HICANN::set_syndriver_switch_row(this->h, addr, srow_cfg);

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
	HICANN::set_synapse_driver(this->h, SynapseDriverOnHICANN(Y(111), left), driver);

	// set ANALOG
	HICANN::Analog ac;
	ac.enable(AnalogOnHICANN(0));
	ac.set_preout(AnalogOnHICANN(0));
	HICANN::set_analog(this->h, ac);

	char c;
	bool success = false;
	bool cont = true;
	while (cont) {
		std::cout << "Do you see s.th. on the analog output? (y/n)" << std::endl << std::flush;
		std::cin >> c;
		if (c == 'y' ) {
			success=true;
			cont =false;
		}
		else if (c == 'n' ) {
			success=false;
			cont =false;
		}
	}
	EXPECT_GETTER_TRUE(success) << " Current input on the analog output not seen .";
}

template <typename T>
size_t HICANNBackendTest<T>::L1TransmissionTest(RepeaterBlockOnHICANN const block) {
	HICANN::init(this->h, false);

	///period between events used in testing below
	uint16_t period = 150;

	//set PLL frequency
	//HICANN::set_PLL_frequency(this->h, 100);

	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	for (size_t  i = 0; i < 8; i++){
		DNCMergerOnHICANN mer(i);
		mergers[mer].config = HICANN::Merger::RIGHT_ONLY;
		mergers[mer].slow = true;
		mergers[mer].loopback = false;
	}
	HICANN::set_dnc_merger(this->h, mergers);

	//configure merger tree, phase
	HICANN::MergerTree tree; //default settings are OK
	HICANN::Phase phase = 0;
	HICANN::set_merger_tree(this->h, tree);
	HICANN::set_phase(this->h, phase);

	//configure background generators
	HICANN::BackgroundGeneratorArray bgarray;
	for (size_t i = 0; i < 8; i++){
		bgarray[i].enable(true);
		bgarray[i].random(false);
		bgarray[i].seed(200);
		bgarray[i].period(period);
		bgarray[i].address(HICANN::L1Address(0));
	}
	HICANN::set_background_generator(this->h, bgarray);

	//configure sending repeaters
	HICANN::set_repeater_block(this->h, RepeaterBlockOnHICANN(X(0), Y(1)), HICANN::RepeaterBlock());
	HICANN::HorizontalRepeater sr;
	sr.setOutput(right);
	for (size_t i = 0; i < 8; i++) HICANN::set_repeater(this->h, HRepeaterOnHICANN(HLineOnHICANN(6+8*i), left), sr);

	//configure crossbars and synapse switches
	HICANN::Crossbar cb;
	HICANN::SynapseSwitch ss;
	HICANN::CrossbarRow row_cfg;
	HICANN::SynapseSwitchRow syn_row_cfg;

	// Dare not to copy this! Use block.x() and block.y() for new code!
	enum {
		topleft,
		topright,
		centerleft,
		centerright,
		bottomleft,
		bottomright
	};

	for (size_t i = 0; i < 8; i++) {
		switch (block.id()) {
			case bottomleft : {
				cb.set(VLineOnHICANN(28-4*i), HLineOnHICANN(6+8*i), true);
				row_cfg = cb.get_row(HLineOnHICANN(6+8*i), left);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6+8*i), left, row_cfg);
			} break;
			case bottomright : {
				cb.set(VLineOnHICANN(227+4*i), HLineOnHICANN(6+8*i), true);
				row_cfg = cb.get_row(HLineOnHICANN(6+8*i), right);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6+8*i), right, row_cfg);
			} break;
			case topleft : {
				SynapseSwitchRowOnHICANN addr(Y(15-2*i), left);
				cb.set(VLineOnHICANN(28-4*i), HLineOnHICANN(6+8*i), true);
				ss.set(VLineOnHICANN(28-4*i), addr.line(), true); //connecting two parallel vertical lanes
				ss.set(VLineOnHICANN(29-4*i), addr.line(), true);
				row_cfg = cb.get_row(HLineOnHICANN(6+8*i), left);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6+8*i), left, row_cfg);
				syn_row_cfg = ss.get_row(addr);
				HICANN::set_syndriver_switch_row(this->h, addr, syn_row_cfg);
			} break;
			case topright : {
				SynapseSwitchRowOnHICANN addr(Y(15-2*i), right);
				cb.set(VLineOnHICANN(227+4*i), HLineOnHICANN(6+8*i), true);
				ss.set(VLineOnHICANN(227+4*i), addr.line(), true); //connecting two parallel vertical lanes
				ss.set(VLineOnHICANN(226+4*i), addr.line(), true);
				row_cfg = cb.get_row(HLineOnHICANN(6+8*i), right);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6+8*i), right, row_cfg);
				syn_row_cfg = ss.get_row(addr);
				HICANN::set_syndriver_switch_row(this->h, addr, syn_row_cfg);
			} break;
			case centerright : {
				cb.set(VLineOnHICANN(28-4*i), HLineOnHICANN(6+8*i), true);
				cb.set(VLineOnHICANN(28-4*i), HLineOnHICANN(7+8*i), true);
				row_cfg = cb.get_row(HLineOnHICANN(6+8*i), left);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(6+8*i), left, row_cfg);
				row_cfg = cb.get_row(HLineOnHICANN(7+8*i), left);
				HICANN::set_crossbar_switch_row(this->h, HLineOnHICANN(7+8*i), left, row_cfg);
			} break;
		}
	}

	//readout-routine
	HICANN::VerticalRepeater vr = HICANN::VerticalRepeater();
	HICANN::RepeaterBlock rb = HICANN::RepeaterBlock();
	size_t startrep = 0;
	TestPortOnRepeaterBlock tp(0);
	SideVertical dir = top;

	switch (block.id()) {
		case bottomleft : {
			startrep = 0;
			tp = TestPortOnRepeaterBlock(0);
			dir = bottom;
		} break;
		case bottomright : {
			startrep = 227;
			tp = TestPortOnRepeaterBlock(0);
			dir = bottom;
		} break;
		case topleft : {
			startrep = 1;
			tp = TestPortOnRepeaterBlock(1);
			dir = top;
		} break;
		case topright : {
			startrep = 226;
			tp = TestPortOnRepeaterBlock(1);
			dir = top;
		} break;
	}

	size_t success_counter = 0;

	// unfortunately center right readout routine is too different...
	if (block.id() != centerright) {
		for (size_t i = startrep; i < startrep + 32; i+=4) {
			//configure receiving repeater
			vr.setInput(dir);
			HICANN::set_repeater(this->h, VLineOnHICANN(i).toVRepeaterOnHICANN(), vr);

			usleep(500); //time for the dll to lock

			//configure receiving repeater block
			rb.start_tdi[tp] = false; //reset the full flag
			HICANN::set_repeater_block(this->h, block, rb);
			rb.start_tdi[tp] = true; //start recording received data
			HICANN::set_repeater_block(this->h, block, rb);

			usleep(1000);  //recording lasts for ca. 4 µs - 1ms

			rb = HICANN::get_repeater_block(this->h, block);
			std::array<HICANN::RepeaterBlock::TestEvent, 3> test = rb.tdi_data[tp];

			for (size_t x = 0; x < 2; x++) //see how many events come with correct delay (+/- 2)
				if((std::abs( std::abs(test[x+1].time - test[x].time) - period )) < 3)
					success_counter++;

			//~ cout << dec << "From repeater " << i << " received: ";
			//~ if (rb.full_flag[tp]) cout << "Full flag is set" << endl;
			//~ else cout << "Full flag is NOT set" << endl;
			//~ cout << "Neuron number " << (uint32_t)test[0].address << " at time " << (uint32_t)test[0].time << endl;
			//~ cout << "Neuron number " << (uint32_t)test[1].address << " at time " << (uint32_t)test[1].time << endl;
			//~ cout << "Neuron number " << (uint32_t)test[2].address << " at time " << (uint32_t)test[2].time << endl << endl;

			rb.start_tdi[tp] = false; //reset the full flag TWO (!) times
			HICANN::set_repeater_block(this->h, block, rb);
			HICANN::set_repeater_block(this->h, block, rb);

			//set repeater mode to IDLE to prevent conflicts
			vr.setIdle();
			HICANN::set_repeater(this->h, VLineOnHICANN(i).toVRepeaterOnHICANN(), vr);
		}
	} else {
		HICANN::HorizontalRepeater hr = HICANN::HorizontalRepeater();
		HICANN::RepeaterBlock rb = HICANN::RepeaterBlock();
		for (size_t i = 7; i < 64; i+=8) {
			//configure receiving repeater
			hr.setInput(right);
			HICANN::set_repeater(this->h, HLineOnHICANN(i).toHRepeaterOnHICANN(), hr);

			usleep(500); //time for the dll to lock

			//configure receiving repeater block
			rb.start_tdi[0] = false; //reset the full flag
			HICANN::set_repeater_block(this->h, block, rb);
			rb.start_tdi[0] = true; //start recording received data
			HICANN::set_repeater_block(this->h, block, rb);

			usleep(1000);  //recording lasts for ca. 4 µs - 1ms

			rb = HICANN::get_repeater_block(this->h, block);
			std::array<HICANN::RepeaterBlock::TestEvent, 3> test = rb.tdi_data[0];

			//~ cout << dec << "From repeater " << i << " received: ";
			//~ if (rb.full_flag[0]) cout << "Full flag is set" << endl;
			//~ else cout << "Full flag is NOT set" << endl;
			//~ cout << "Neuron number " << (uint32_t)test[0].address << " at time " << (uint32_t)test[0].time << endl;
			//~ cout << "Neuron number " << (uint32_t)test[1].address << " at time " << (uint32_t)test[1].time << endl;
			//~ cout << "Neuron number " << (uint32_t)test[2].address << " at time " << (uint32_t)test[2].time << endl << endl;

			for (size_t x = 0; x < 2; x++) //see how many events come with correct delay
				if((std::abs( std::abs(test[x+1].time - test[x].time) - period )) < 3)
					success_counter++;

			rb.start_tdi[0] = false; //reset the full flag TWO (!) times
			HICANN::set_repeater_block(this->h, block, rb);
			HICANN::set_repeater_block(this->h, block, rb);

			//set repeater mode to IDLE to prevent conflicts
			hr.setIdle();
			HICANN::set_repeater(this->h, HLineOnHICANN(i).toHRepeaterOnHICANN(), hr);
		}
	}
	//~ cout << success_counter << " successful reads" << endl;

	return success_counter;
}

TYPED_TEST(HICANNBackendTest, HICANNL1BottomLeftRepeaterHWTest) {
	size_t count = this->L1TransmissionTest(RepeaterBlockOnHICANN(X(0), Y(2)));
	EXPECT_GETTER_GE(count, 3); //3 or more events with correct delay (out of 16) are enough
}

TYPED_TEST(HICANNBackendTest, HICANNL1BottomRightRepeaterHWTest) {
	size_t count = this->L1TransmissionTest(RepeaterBlockOnHICANN(X(1), Y(2)));
	EXPECT_GETTER_GE(count, 3); //3 or more events with correct delay (out of 16) are enough
}

TYPED_TEST(HICANNBackendTest, HICANNL1TopLeftRepeaterHWTest) {
	size_t count = this->L1TransmissionTest(RepeaterBlockOnHICANN(X(0), Y(0)));
	EXPECT_GETTER_GE(count, 3); //3 or more events with correct delay (out of 16) are enough
}

TYPED_TEST(HICANNBackendTest, HICANNL1TopRightRepeaterHWTest) {
	size_t count = this->L1TransmissionTest(RepeaterBlockOnHICANN(X(1), Y(0)));
	EXPECT_GETTER_GE(count, 3); //3 or more events with correct delay (out of 16) are enough
}

TYPED_TEST(HICANNBackendTest, HICANNL1CenterRightRepeaterHWTest) {
	size_t count = this->L1TransmissionTest(RepeaterBlockOnHICANN(X(1), Y(1)));
	EXPECT_GETTER_GE(count, 3); //3 or more events with correct delay (out of 16) are enough
}

TYPED_TEST(HICANNBackendTest, ResetHostConnectionHWTest) {
	// typical flow...
	HICANN::init(this->h, false);

	// dummy-resets... noops! illegal now!
	FPGA::Reset r(/*false*/true);
	for (size_t i = 0; i < 10; i++)
		FPGA::reset(this->f, r);

	// reset + inits loop
	for (size_t i = 0; i < 3; i++) {
		FPGA::reset(this->f);
		HICANN::init(this->h, false);
	}

	HICANN::flush(this->h);

	// now some pure connection resets
	r.core = true; // we have to reset the core too...
	// BV: flag host_connection has been removed, as host connections must be reset with every core reset 
	// r.host_connection = true;
	for (size_t i = 0; i < 10; i++)
		FPGA::reset(this->f, r);

	// inits loop (failed before)
	for (size_t i = 0; i < 3; i++) {
		HICANN::init(this->h, false);
	}
}
#undef RET
#undef HCL1
#undef HCFG
#undef HCREP
#undef HCSYN
} // namespace HMF
