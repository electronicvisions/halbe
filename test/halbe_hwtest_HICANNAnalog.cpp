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
using namespace HMF::Coordinate;

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

TEST_F(HICANNAnalogTest, DISABLED_FGResponseCurveHWTest) {
	HICANN::init(h, false);

	HICANN::FGControl fgc = HICANN::FGControl();
	FGBlockOnHICANN block {Enum{0}};

	HICANN::FGConfig config;
	config.pulselength = 15;
	config.fg_biasn = 8;
	config.voltagewritetime = 2;

	HICANN::neuron_parameter param = HICANN::V_syntci; //what parameter (line) to use
	size_t runs = 30, times_to_write = 2;
	std::vector<vector<double> > sums(1024);
	std::vector<uint16_t> linevalues, tempvalues;
	double temp;

	for (size_t n = 0; n < 1024; n++) sums[n].clear();

	//configure analog output
	HICANN::Analog aout;
	aout.set_fg_left(AnalogOnHICANN(0));
	aout.disable(AnalogOnHICANN(1));
	HICANN::set_analog(h, aout);

	//configure ADC
	HMF::ADC::Config conf = HMF::ADC::Config(980, chan0, trig);
	HMF::ADC::config(adc, conf);

	for (size_t s = 0; s < runs; s++) { //main runs counter
		cout << endl << "Run " << s+1 << " of " << runs << endl << endl;
		randomizeVector(linevalues, tempvalues);

		for (size_t w = 0; w < times_to_write; w++) { //write FG line (parameter)
			for (size_t neu = 0; neu < 128; neu++) {
				fgc.setNeuron(NeuronOnHICANN(Enum(neu)), param, linevalues[neu+1]);
			}

			//HICANN::set_PLL_frequency(h, 100);
			HICANN::set_fg_config(h, block, config);
			HICANN::set_fg_values(h, block, fgc.getBlock(block));
			//HICANN::set_PLL_frequency(h, 200);
		}

		cout << endl << "read back from FGs:" << endl;

		for (size_t neu = 0; neu < 128; neu++) { //all 128 neurons (values of one  FG line)
			HICANN::set_fg_cell(h, NeuronOnHICANN(Enum(neu)), param);
			HICANN::flush(h);
			HMF::ADC::trigger_now(adc);
			temp = cal.get_voltage(VecInfo(HMF::ADC::get_trace(adc)).mean());

			sums[linevalues[neu+1]].push_back(temp);
			cout << fixed << setprecision(3) << temp << "\t" << flush;
		}
		cout << endl;
	}

	write_files(sums, "measurement", "rawdatameasurement");
}

TEST_F(HICANNAnalogTest, DISABLED_FGParamSweepHWTest) {
	HICANN::init(h, false);

	size_t sw1, sw2, fgn, line, start1 = 0, end1 = 0, step1 = 1, start2, end2, step2;
	stringstream sweepparam1, rawdata1, sweepparam2, rawdata2;
	size_t runs = 100, times_to_write = 2;
	std::vector<vector<double>> sums(1024);
	std::vector<uint16_t> linevalues, tempvalues;

	cout << "Which controller? 0 - 3" << endl;
	cin >> fgn;
	cout << "Which line? 0 - 23" << endl;
	cin >> line;
	cout << "How many runs per sweep?" << endl;
	cin >> runs;

	cout << "Choose first parameter to sweep: " << endl;
	cout << "1: Voltagewritetime" << endl;
	cout << "2: Currentwritetime" << endl;
	cout << "3: Acceleratorstep" << endl;
	cout << "4: Pulselength" << endl;
	cout << "5: Readtime" << endl;
	cout << "6: Biasn" << endl;
	cout << "7: Biasp" << endl;
	cout << "8: Line number" << endl;
	cout << "9: FG controler" << endl;
	cout << "0: None" << endl;
	cin >> sw1;
	if (sw1) { //omitted if single-sweep => no loop
		cout << "Choose starting value: " << endl;
		cin >> start1;
		cout << "Choose end value: " << endl;
		cin >> end1;
		cout << "Choose sweep step: " << endl;
		cin >> step1;
	}

	cout << "Choose second parameter to sweep: " << endl;
	cout << "1: Voltagewritetime" << endl;
	cout << "2: Currentwritetime" << endl;
	cout << "3: Acceleratorstep" << endl;
	cout << "4: Pulselength" << endl;
	cout << "5: Readtime" << endl;
	cout << "6: Biasn" << endl;
	cout << "7: Biasp" << endl;
	cout << "8: Line number" << endl;
	cout << "9: FG controler" << endl;
	cin >> sw2;
	cout << "Choose starting value: " << endl;
	cin >> start2;
	cout << "Choose end value: " << endl;
	cin >> end2;
	cout << "Choose sweep step: " << endl;
	cin >> step2;

	//load initial values
	HICANN::Analog aout;
	HMF::ADC::Config conf = HMF::ADC::Config(980, chan0, trig);
	FGBlockOnHICANN block {Enum{fgn}};
	HICANN::FGBlock fgblock;

	HICANN::FGConfig config;
	config.maxcycle         = 255;
	config.currentwritetime = 1;
	config.voltagewritetime = 2; //(3?)
	config.readtime         = 63;
	config.acceleratorstep  = 21;
	config.pulselength      = 8;
	config.fg_biasn         = 9;
	config.fg_bias          = 8;

	for(size_t num1 = start1; num1 <= end1; num1 += step1){
		sweepparam1.str("");
		rawdata1.str("");
		if      (sw1 == 1) {sweepparam1 << "volttime" << num1;    rawdata1 << "rawdatavolttime" << num1;    config.voltagewritetime=num1;}
		else if (sw1 == 2) {sweepparam1 << "currtime" << num1;    rawdata1 << "rawdatacurrtime" << num1;    config.currentwritetime=num1;}
		else if (sw1 == 3) {sweepparam1 << "accstep" << num1;     rawdata1 << "rawdataaccstep" << num1;     config.acceleratorstep=num1;}
		else if (sw1 == 4) {sweepparam1 << "pulselength" << num1; rawdata1 << "rawdatapulselength" << num1; config.pulselength=num1;}
		else if (sw1 == 5) {sweepparam1 << "readtime" << num1;    rawdata1 << "rawdatareadtime" << num1;    config.readtime=num1;}
		else if (sw1 == 6) {sweepparam1 << "biasn" << num1;       rawdata1 << "rawdatabiasn" << num1;       config.fg_biasn=num1;}
		else if (sw1 == 7) {sweepparam1 << "bias" << num1;        rawdata1 << "rawdatabias" << num1;        config.fg_bias=num1;}
		else if (sw1 == 8) {sweepparam1 << "linenumber" << num1;  rawdata1 << "rawdatalinenumber" << num1;  line=num1;}
		else if (sw1 == 9) {sweepparam1 << "fgcontroler" << num1; rawdata1 << "rawdatafgcontroler" << num1; fgn=num1;}

		for(size_t num2 = start2; num2 <= end2; num2 += step2){
			sweepparam2.str("");
			rawdata2.str("");
			if      (sw2 == 1) {sweepparam2 << "volttime" << num2;    rawdata2 << "rawdatavolttime" << num2;    config.voltagewritetime=num2;}
			else if (sw2 == 2) {sweepparam2 << "currtime" << num2;    rawdata2 << "rawdatacurrtime" << num2;    config.currentwritetime=num2;}
			else if (sw2 == 3) {sweepparam2 << "accstep" << num2;     rawdata2 << "rawdataaccstep" << num2;     config.acceleratorstep=num2;}
			else if (sw2 == 4) {sweepparam2 << "pulselength" << num2; rawdata2 << "rawdatapulselength" << num2; config.pulselength=num2;}
			else if (sw2 == 5) {sweepparam2 << "readtime" << num2;    rawdata2 << "rawdatareadtime" << num2;    config.readtime=num2;}
			else if (sw2 == 6) {sweepparam2 << "biasn" << num2;       rawdata2 << "rawdatabiasn" << num2;       config.fg_biasn=num2;}
			else if (sw2 == 7) {sweepparam2 << "bias" << num2;        rawdata2 << "rawdatabias" << num2;        config.fg_bias=num2;}
			else if (sw2 == 8) {sweepparam2 << "linenumber" << num2;  rawdata2 << "rawdatalinenumber" << num2;  line=num2;}
			else if (sw2 == 9) {sweepparam2 << "fgcontroler" << num2; rawdata2 << "rawdatafgcontroler" << num2; fgn=num2;}
			cout << endl << sweepparam1.str() << " " << sweepparam2.str() << endl << endl;

			//load next parameters
			block = FGBlockOnHICANN(Enum(fgn));

			aout.disable(AnalogOnHICANN(0));
			aout.disable(AnalogOnHICANN(1));
			//configure analog output
			if      (fgn == 0) { aout.set_fg_left(AnalogOnHICANN(0)); }
			else if (fgn == 1) { aout.set_fg_right(AnalogOnHICANN(0)); }
			else if (fgn == 2) { aout.set_fg_left(AnalogOnHICANN(1)); }
			else if (fgn == 3) { aout.set_fg_right(AnalogOnHICANN(1)); }
			HICANN::set_analog(h, aout);

			//configure ADC
			conf = (fgn == 0 || fgn == 1)? HMF::ADC::Config(980, chan0, trig) : HMF::ADC::Config(980, chan1, trig);
			HMF::ADC::config(adc, conf);

			for (size_t n = 0; n < 1024; n++) sums[n].clear(); //clear vectors

			for (size_t s = 0; s < runs; s++) { //main runs counter
				cout << endl << "Run " << s+1 << " of " << runs << endl << endl;
				randomizeVector(linevalues, tempvalues);

				for (size_t w = 0; w < times_to_write; w++) { //write FG line (parameter)
					fgblock.setSharedRaw(line, linevalues[0]);

					for (size_t neu = 0; neu < 128; neu++) {
						bool const is_left = (block.x() == 0);
						fgblock.setNeuronRaw(is_left ? neu : 127-neu, line, linevalues[neu]);
					}
					HICANN::set_fg_config(h, block, config);
					HICANN::set_fg_values(h, block, fgblock);
				}

				cout << endl << "read back from FGs:" << endl;

				for (size_t col = 1; col < 129; col++) { //not the global value (OP cannot read it properly)
					HICANN::set_fg_cell(h, block, FGCellOnFGBlock(X(col), Y(line)));
					HICANN::flush(h);
					HMF::ADC::trigger_now(adc);
					double temp = cal.get_voltage(VecInfo(HMF::ADC::get_trace(adc)).mean());

					sums[linevalues[col]].push_back(temp);
					cout << fixed << setprecision(3) << temp << "\t" << flush;
				}
				cout << endl;
			}

			stringstream sstr1, sstr2;
			sstr1 << sweepparam1.str() << sweepparam2.str();
			sstr2 << rawdata1.str() << rawdata2.str();
			write_files(sums, sstr1.str(), sstr2.str());
		}
	}
}

} // namespace HMF
