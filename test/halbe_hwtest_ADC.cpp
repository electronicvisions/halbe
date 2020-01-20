#include "hwtest.h"

#include "hal/backend/ADCBackend.h"
#include "hal/Handle/ADCHw.h"


//file IO
#include <fstream> //stream to files
#include <iterator> //iterator for stream
//IO
#include <iostream>
#include <stdio.h>
//timing
#include <ctime>
#include <chrono>

//container
#include <vector>
//math includes
#include <cmath>//pow

using namespace std;
using namespace halco::hicann::v2;
using namespace halco::common;

//additional functions for tests---------------------------------------------------------------
namespace math{

//return the nth statistical moment
	template <int grade, typename T_out, typename T_in>
	T_out moment( vector<T_in> vec_){
		const int len_=vec_.size();
		T_out sum_=0;
		for(int i=0;i<len_;++i){
			sum_+=pow((T_out)vec_[i],grade);
		}

		return (sum_/len_);
	}
}

//application of calibration functions-----------------------------------------------------------
namespace calibration{

//TODO: implement input channel specific calibration
//TODO: maybe use the worse linear model instead ?!

template <typename arg_type>
	float to_VOLT(arg_type count_){
		return (2.3139-0.000821*((float)count_)+pow(0.000239*((float)count_),2)-pow(0.0002*((float)count_),3));
	}

template <typename arg_type>
	float to_mVOLT(arg_type count_){
		return 1000*calibration::to_VOLT< arg_type >(count_);
	}

}
//----------------------------------------------------------------------------------------------
namespace HMF {

//---TEST FIXTURES---------------------------------------------------------------------------

//test fixture for ADC based tests
class ADCTest : public ::HWTest {};


//------------------------------------------------------------------------------


//test floating gate array programming linewise
TEST_F(ADCTest,FGprogramHWTest){

//***CONNECT TO HICANN ---------------------------------------------------------
//init hicann handler
	//reset system completely
    HICANN::init(h,false);

//*** CONNECT TO ADC ---------------------------------------------------------
//get adc handler
    Handle::ADCHw adc;
//configure adc readout
    ADC::config(adc,ADC::Config(9711 /*=~100 usec*/,ChannelOnADC(3),TriggerOnADC(0)));
	usleep(1000);
//configure analog output of system emulator board and hicann
	HICANN::Analog aout;
	aout.set_fg_left(AnalogOnHICANN(0));//get floating gate signal to right output
	aout.enable(AnalogOnHICANN(0));//enable right output
	aout.enable(AnalogOnHICANN(1));//disable left output
    HICANN::set_analog(h, aout);//configure output
	usleep(1000);

//*** CONFIGURE FLOATING GATES -------------------------------------------------

//enumerate all parameters of a neuron
//get voltage gates
	vector<int> volt_fg_num_vec;
	volt_fg_num_vec.push_back(6);//E_l
	volt_fg_num_vec.push_back(8);//V_syni
	volt_fg_num_vec.push_back(10);//V_syntci
	volt_fg_num_vec.push_back(12);//V_t
	volt_fg_num_vec.push_back(14);//V_syntcx
	volt_fg_num_vec.push_back(16);//E_synx
	volt_fg_num_vec.push_back(18);//E_syni
	volt_fg_num_vec.push_back(20);//V_exp
	volt_fg_num_vec.push_back(22);//V_synx
	
//get current gates
	vector<int> curr_fg_num_vec;
	curr_fg_num_vec.push_back(1);//I_bexp
	curr_fg_num_vec.push_back(3);//I_convi
	curr_fg_num_vec.push_back(5);//I_spikeamp
	curr_fg_num_vec.push_back(7);//I_fire
	curr_fg_num_vec.push_back(9);//I_gladapt
	curr_fg_num_vec.push_back(11);//I_gl
	curr_fg_num_vec.push_back(13);//I_pl
	curr_fg_num_vec.push_back(15);//I_radapt
	curr_fg_num_vec.push_back(17);//I_convx
	curr_fg_num_vec.push_back(19);//I_intbbx
	curr_fg_num_vec.push_back(21);//I_intbbi
	curr_fg_num_vec.push_back(23);//I_rexp

const int VOLTVECSIZE=volt_fg_num_vec.size();
const int CURRVECSIZE=curr_fg_num_vec.size();

//*** DATA FOR STATISTIC TEST------------------------------------------------------
vector <int> prog_val_vec;
vector <float> volt_exp_val_vec;
vector <float> volt_tolerance_vec;

vector <float> curr_exp_val_vec;
vector <float> curr_tolerance_vec;

//200
prog_val_vec.push_back(200);

volt_exp_val_vec.push_back(0.338);
volt_tolerance_vec.push_back(3*0.01);

curr_exp_val_vec.push_back(0.338);
curr_tolerance_vec.push_back(3*0.015);

//400
prog_val_vec.push_back(400);

volt_exp_val_vec.push_back(0.6986);
volt_tolerance_vec.push_back(3*0.015);

curr_exp_val_vec.push_back(0.6903);
curr_tolerance_vec.push_back(3*0.015);

//600
prog_val_vec.push_back(600);

volt_exp_val_vec.push_back(1.0223);
volt_tolerance_vec.push_back(3*0.02);

curr_exp_val_vec.push_back(1.0299);
curr_tolerance_vec.push_back(3*0.02);

//impedance 
const float mu =1.036;//correct read voltage due to impedance effects

//*** MAKE STATISTIC TEST ON FG LINES -------------------------------------------
//main iteration
for(size_t run=0;run<prog_val_vec.size();++run){

	//pack configuration for a floating gate block
		HMF::HICANN::FGBlock fg;
		HMF::HICANN::FGConfig fg_cfg;
		auto b = FGBlockOnHICANN(Enum(0));
	//program all floatings gates to one value
		for(int i=0;i<128;++i){
			for(int j=0;j<24;++j){
				fg.setNeuronRaw(i, j, prog_val_vec[run]);
			};  
		}; 
	//program twice
		HICANN::set_fg_config(h,b, fg_cfg);
		HICANN::set_fg_values(h,b,fg);
		HICANN::set_fg_values(h,b,fg);

//*** VOLT PARAMETER PART-----------------------------------------------------------
	for(int para_num=0;para_num<VOLTVECSIZE;++para_num){
		vector<float> mean_vec;
		for(int neuron_num=1;neuron_num<129;++neuron_num){
	//listen to selected floating gate
			HICANN::set_fg_cell(h,b, FGCellOnFGBlock(X(neuron_num), Y(volt_fg_num_vec[para_num])));
	//wait after switching
			usleep(1000);
	//trigger readout
			ADC::trigger_now(adc);
	//wait till read
			usleep(1000);
	//get recorded trace
			vector<ADC::raw_type> data= ADC::get_trace(adc);
	//calculate mean value of floating gate
			float mean = math::moment< 1,float,ADC::raw_type >(data);
	//accumulate in mean_vec
			mean_vec.push_back(calibration::to_VOLT<float>(mean));
		};
	//calculate statistics
		float mean_of_means=math::moment<1,float,float>(mean_vec);
	//compare to expectation
		ASSERT_NEAR(volt_exp_val_vec[run],mean_of_means*mu,volt_tolerance_vec[run]);
	};

//*** CURR PARAMETER PART------------------------------------------------------------
	for(int para_num=0;para_num<CURRVECSIZE;++para_num){
		vector<float> mean_vec;
		for(int neuron_num=1;neuron_num<129;++neuron_num){
	//listen to selected floating gate
			HICANN::set_fg_cell(h,b, FGCellOnFGBlock(X(neuron_num), Y(curr_fg_num_vec[para_num])));
	//wait after switching
			usleep(1000);
	//trigger readout
			ADC::trigger_now(adc);
	//wait till read
			usleep(1000);
	//get recorded trace
			vector<ADC::raw_type> data= ADC::get_trace(adc);
	//calculate mean value of floating gate
			float mean = math::moment< 1,float,ADC::raw_type >(data);
	//accumulate in mean_vec
			mean_vec.push_back(calibration::to_VOLT<float>(mean));
		};
	//calculate statistics
		float mean_of_means=math::moment<1,float,float>(mean_vec);
	//compare to expectation
		ASSERT_NEAR(curr_exp_val_vec[run],mean_of_means*mu,curr_tolerance_vec[run]);
	};
};//end run loop

}//TEST_F FGprogram_HWTest

TEST_F(ADCTest, TracePerfHWTest){
    Handle::ADCHw adc;

	/* pairs of number of samples and expected mega samples / second */
	vector< pair<size_t, double>> experiment_param {
	   {10e3,1e6}, {100e3,10e6}, {1e6,10e6}, {10e6,15e6}, {100e6,15e6}
	};

	/* loop over all test runs*/
	for (auto i : experiment_param) {
		ADC::config(adc, ADC::Config(i.first, ChannelOnADC(0), TriggerOnADC(0)));
		ADC::trigger_now(adc);
		auto start = chrono::high_resolution_clock::now();
		vector<ADC::raw_type> data = ADC::get_trace(adc);
		/* check if received all samples*/
		EXPECT_EQ(data.size(), i.first);
		auto end = chrono::high_resolution_clock::now();
		size_t runtime = chrono::duration_cast<chrono::microseconds>(end - start).count();
		EXPECT_GE(1e6 * i.first / runtime, i.second);
	}
}

} // namespace HMF
