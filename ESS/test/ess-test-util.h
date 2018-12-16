#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include "logger.h"

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Handle/FPGAEss.h"
#include "hal/Handle/Ess.h"
#include "ESS/halbe_to_ess.h"

//from test-util.h (halbe/test) for Test_Weight_Config
template <typename T>
class IncrementingSequence
{
public:
    // Constructor, just set counter to 0
    IncrementingSequence(int max) : i_(0), max_i_(max) {}
    // Return an incrementing number
    T operator() ()
    {
		if (i_ == max_i_+1) i_ = 0; //reset value
		return T(i_++);
	}
private:
    int i_;
    int max_i_;
};

//from test-util.h (halbe/test) for test-RUNESS
class ConstantSequence
{
public:
    ConstantSequence(int x) : x_(x) {}
    int operator() () { return x_; }
private:
    int x_;
};

template <size_t length>
bool is_element_array(std::array<size_t, length> arr, size_t addr)
{
	bool returnval = false;

	for(size_t i = 0; i < arr.size(); ++i)
	{
		if(arr[i] == addr)
			returnval = true;
	}

	return returnval;
}

namespace HMF
{
//provides the test-fixture
class ESSTest : public ::testing::Test {
protected:
	ESSTest();
	~ESSTest();

//	virtual void SetUp();
	// brauche TearDown()??

	boost::shared_ptr<Handle::Ess> ess;

	Handle::FPGAEss fpga;
	Handle::HICANNEss & h;
};

//functions to load specific neuron parameter configurations
void load_pattern_naud(HICANN::FGControl &fgc, size_t neuron, std::string const& pattern);
void load_pattern(HICANN::FGControl &fgc, size_t neuron, std::string const& pattern);
}

class ESSTestEnviroment : public ::testing::Environment {
public:
	// TODO check if still necessary
	ESSTestEnviroment()
	{}
};



