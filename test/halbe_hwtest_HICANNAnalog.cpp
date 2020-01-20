#include <gtest/gtest.h>

#include <iostream>
#include <time.h>

#include "hwtest.h"
#include "hal/backend/ADCBackend.h"
#include "hal/Handle/ADCHw.h"
#include "hal/HICANN/FGControl.h"

#include <cmath>
#include <ctime>
#include <numeric>
#include <fstream>
#include <string>
#include <iostream>

using namespace std;
using namespace halco::hicann::v2;
using namespace halco::common;

namespace HMF {

//ADC quick linear calibration earmarks Endvalue=(a*value+b)
struct ADCCalib {
	ADCCalib() : a(-0.000694), b(2.1) {} //standard values
	double a, b;
	double get_voltage(double val) { return a*val + b; }
};

//store temporary calibration data globally here
static HMF::ADCCalib cal = ADCCalib();

//helper class for calculating mean / standard deviation of a vector
class VecInfo
{
public:
	VecInfo(const std::vector<uint16_t> & vec) : data(vec.begin(), vec.end())
	{
		mymean = accumulate(data.begin(), data.end(), 0) / (double) data.size();

		transform(data.begin(), data.end(), data.begin(), bind2nd(minus<double>(), mymean));
		mystdev = inner_product(data.begin(), data.end(), data.begin(), 0.0);
		mystdev = sqrt(mystdev / (double) (data.size()-1));
	}

	VecInfo(const std::vector<double> & vec) : data(vec)
	{
		mymean = accumulate(data.begin(), data.end(), 0.0) / (double) data.size();

		transform(data.begin(), data.end(), data.begin(), bind2nd(minus<double>(), mymean));
		mystdev = inner_product(data.begin(), data.end(), data.begin(), 0.0);
		mystdev = sqrt(mystdev / (double) (data.size()-1));
	}

	double mean() { return mymean; }

	double stdev() { return mystdev; }

	friend std::ostream& operator<< (std::ostream& os, const VecInfo& i)
	{
		os << "Vector size = " << i.data.size() <<
		" With mean = " << setprecision(2) << fixed << i.mymean <<
		" and STD = " << setprecision(2) << fixed << i.mystdev << endl;
		return os;
	}

private:
	vector<double> data;
	double mymean;
	double mystdev;
};


//generates a vector of values from 0 to 1023 that are randomly arranged
std::vector<uint16_t> generate_rnd(){
	std::vector<uint16_t> vec(1024);
	std::iota(vec.begin(), vec.end(), 0);
	std::random_shuffle(vec.begin(), vec.end());
	return vec;
}

//generates a line of random 129 values that can be written to an FG line
void randomizeVector(std::vector<uint16_t>& linevalues, std::vector<uint16_t>& tempvalues){
	cout << "Random vector is:" << endl;
	linevalues.clear();
	for (size_t b = 0; b < 129; b++){
		if (tempvalues.empty()) tempvalues = generate_rnd();
		linevalues.push_back(tempvalues.back());
		tempvalues.pop_back();
		if (b) cout << fixed << setprecision(3) << ((double)linevalues[b]/1024.0)*1.8 << "\t" << flush;
	}
	cout << endl;
}

void write_files(std::vector<vector<double> > sums, string namefinal, string nameraw){
	stringstream ss;
	fstream file;

	std::vector<double> mean(1024), stdev(1024); // means of sums columns and standard deviations
	ss.str("");
	string directory = "test/results/";
	ss << directory << namefinal << ".dat";
	string filename = ss.str();
	file.open(filename.c_str(), fstream::out | fstream::app);

	for (size_t p = 0; p < 1024; p++){
		VecInfo meanstd = VecInfo(sums[p]);
		mean[p] = meanstd.mean();
		if (sums[p].size() > 1) stdev[p] = meanstd.stdev();
		file << fixed << setprecision(4) << mean[p] << " " << setprecision(5) << stdev[p] << "\n" << flush;
	}
	file << endl;
	file.flush();
	file.close();

	ss.str("");
	ss << directory << nameraw << ".dat";
	filename = ss.str();
	file.open(filename.c_str(), fstream::out | fstream::app);

	for (size_t p = 0; p < 1024; p++){
		for (size_t g = 0; g < sums[p].size(); g++) file << fixed << setprecision(4) << sums[p][g] << " " << flush;
		file <<  "\n" << flush;
	}
	file << endl;
	file.flush();
	file.close();
}



class HICANNAnalogTest : public ::HWTest {
protected:
	HICANNAnalogTest() :
		adc(),         //ADC-handle
		chan0(5),      //ADC channel 0
		chan1(7),      //ADC channel 1
		trig(0)        //ADC trigger-channel
	{}

	Handle::ADCHw  adc;
	ChannelOnADC chan0, chan1;
	TriggerOnADC trig;
};


//quick calibration for the ADC, the other tests depend on it
TEST_F(HICANNAnalogTest, CalibrateADCHWTest) {
	HICANN::init(h, false);

	HICANN::FGConfig fgcfg = HICANN::FGConfig();
	HICANN::FGControl fgc = HICANN::FGControl();
	FGBlockOnHICANN block {Enum{0}};
	NeuronOnHICANN nrn = NeuronOnHICANN(Enum(0));

	//set FG parameters
	fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 1023);

	//write values to hardware
	HICANN::set_fg_config(h, block, fgcfg);
	HICANN::set_fg_values(h, block, fgc.getBlock(block));

	//configure analog output
	HICANN::Analog aout;
	aout.set_fg_left(AnalogOnHICANN(0));
	aout.disable(AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);

	//put out the floating gate cell onto the aout and enable it
	HICANN::set_fg_cell(h, nrn, HICANN::neuron_parameter::I_gl);
	HICANN::flush(h);

	//read out high value with ADC
	HMF::ADC::Config conf = HMF::ADC::Config(980, chan0, trig);
	HMF::ADC::config(adc, conf);
	HMF::ADC::trigger_now(adc);
	VecInfo high = VecInfo(HMF::ADC::get_trace(adc));

	//configure analog output
	aout.disable(AnalogOnHICANN(0));
	HICANN::set_analog(h, aout);
	HICANN::flush(h);

	HMF::ADC::trigger_now(adc);
	VecInfo low = VecInfo(HMF::ADC::get_trace(adc));

	//create calibration curve
	//maximum measurable voltage with 50 Ohm load is ca. 1.57 V =>
	cal.a = 1.57 / (high.mean() - low.mean()); //normalized to 1.57 Volt
	cal.b = -cal.a*low.mean();                //response curve of the ADC is inverted
	cout << "Calibration data: a = " << cal.a << ", b = "<< cal.b << endl;

	//check calibration borders
	EXPECT_TRUE(cal.a < -0.00065 && cal.a > -0.00075 && cal.b < 10.0 && cal.b > -10.0);
}

TEST_F(HICANNAnalogTest, CheckSomeValuesHWTest) {
	HICANN::init(h, false);

	HICANN::FGConfig fgcfg = HICANN::FGConfig();
	HICANN::FGControl fgc = HICANN::FGControl();
	FGBlockOnHICANN block {Enum{0}};
	NeuronOnHICANN nrn;

	//set FG parameters
	fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 600);
	fgc.setNeuron(nrn, HICANN::neuron_parameter::I_intbbx, 300);
	fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 300);

	//write values to hardware
	HICANN::set_fg_config(h, block, fgcfg);
	HICANN::set_fg_values(h, block, fgc.getBlock(block));

	//configure analog output
	HICANN::Analog aout;
	aout.set_fg_left(AnalogOnHICANN(0));
	aout.set_none(AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);
	HICANN::flush(h);

	//configure ADC
	HMF::ADC::Config conf = HMF::ADC::Config(980, chan0, trig);
	HMF::ADC::config(adc, conf);

	//read out the cells
	double E_l, I_gl, I_intbbx;

	HICANN::set_fg_cell(h, nrn, HICANN::neuron_parameter::I_gl);
	HICANN::flush(h);
	HMF::ADC::trigger_now(adc);
	I_gl = cal.get_voltage(VecInfo(HMF::ADC::get_trace(adc)).mean());
	cout << "I_gl = " << fixed << setprecision(3) << I_gl << " V" << endl;

	HICANN::set_fg_cell(h, nrn, HICANN::neuron_parameter::I_intbbx);
	HICANN::flush(h);
	HMF::ADC::trigger_now(adc);
	I_intbbx = cal.get_voltage(VecInfo(HMF::ADC::get_trace(adc)).mean());
	cout << "I_intbbx = " << fixed << setprecision(3) << I_intbbx << " V" << endl;

	HICANN::set_fg_cell(h, nrn, HICANN::neuron_parameter::E_l);
	HICANN::flush(h);
	HMF::ADC::trigger_now(adc);
	E_l = cal.get_voltage(VecInfo(HMF::ADC::get_trace(adc)).mean());
	cout << "E_l = " << fixed << setprecision(3) << E_l << " V" << endl;

	//check if the values make sense
	EXPECT_TRUE(
		I_gl > 0.9 && I_gl < 1.1 &&
		E_l > 0.4 && E_l < 0.6 &&
		I_intbbx > 0.4 && I_intbbx < 0.6
	);
}

} // namespace HMF
