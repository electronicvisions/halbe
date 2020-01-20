#include <gtest/gtest.h>
#include <cstdlib>
#include <bitter/bitter.h>

#include "hal/HICANN/FGConfig.h"
#include "hal/HICANN/FGControl.h"

#include "halco/hicann/v2/neuron.h"

using namespace halco::hicann::v2;
using namespace halco::common;
using namespace std;

namespace HMF {
namespace HICANN {

std::vector<uint16_t>
random_values(size_t n = 24)
{
	std::vector<uint16_t> r(n, 0);
	for (auto& val : r)
		val = rand() % 1024;
	return r;
}

void randomize(FGConfig& config)
{
	config.maxcycle = rand();
	config.readtime = rand();
	config.acceleratorstep = rand();
	config.voltagewritetime = rand();
	config.currentwritetime = rand();
	config.fg_bias = rand();
	config.fg_biasn = rand();
	config.pulselength = rand();
	config.groundvm = rand();
	config.calib = rand();
}

void randomize(FGBlock& fg)
{
	{ // randomize shared parameter first
		size_t cnt = 0;
		auto shared = random_values();
		ASSERT_EQ(24, shared.size());
		for (auto const& val : shared)
		{
			fg.setSharedRaw(cnt, val);
			ASSERT_EQ(val, fg.getSharedRaw(cnt));
			cnt++;
		}
	}

	{ // then neuron parameters
		FGBlockOnHICANN b{Enum{size_t(rand())%4}};
		for (size_t nrn = 0; nrn<128; ++nrn)
		{
			size_t cnt = 0;
			auto param = random_values();
			ASSERT_EQ(24, param.size());
			for (auto const& val : param)
			{
				fg.setNeuronRaw(nrn, cnt, val);
				ASSERT_EQ(val, fg.getNeuronRaw(nrn, cnt));
				cnt++;
			}
		}
	}
}

void randomize(FGControl& fgc)
{
	for (size_t ii=0; ii<4; ++ii)
	{
		FGBlock block;
		randomize(block);

		fgc.setBlock(FGBlockOnHICANN(Enum(ii)), block);
		ASSERT_EQ(block, fgc.getBlock(FGBlockOnHICANN(Enum(ii))));
	}
}

typedef std::array<std::bitset<20>, 65> data_t;

void verify_shared(FGBlock const& fg, data_t const& data, size_t row)
{
	std::bitset<10> val = bit::crop<10>(data.at(0), 0);
	ASSERT_EQ(fg.getSharedRaw(row), val.to_ulong()) << "data[0] = " << data.at(0);
}

void verify_neuron(FGBlockOnHICANN const& b, FGBlock const& fg, data_t const& data, size_t row)
{
	for (size_t nrn = 0; nrn<128; ++nrn)
	{
		// neuron<-> FG row assignment is mirrored on right side
		size_t const nn = fg.is_left(b) ? nrn : 127-nrn;

		std::bitset<20> const d = data.at((nn+1)/2);
		std::bitset<10> const val = bit::crop<10>(d, (nn % 2) ? 0 : 10);

		ASSERT_EQ(fg.getNeuronRaw(nn, row), val.to_ulong())
			<< "data = " << d << " side = " << b.x() << " nrn: " << nrn;

		auto const& lut = fg.getNeuronLut(b);
		auto it = std::find(lut.begin(), lut.end(), row);
		if (it != lut.end()) {
			ASSERT_EQ(fg.getNeuron(b, NeuronOnFGBlock(nrn),
				neuron_parameter(it-lut.begin())), val.to_ulong());
		}
	}
}

void verify_bias(FGConfig const& config, std::bitset<14> const& data)
{
	ASSERT_EQ(config.fg_bias, bit::crop<4>(data, 0));
	ASSERT_EQ(config.fg_biasn, bit::crop<4>(data, 4));
	ASSERT_EQ(config.pulselength, bit::crop<4>(data, 8));
	ASSERT_EQ(config.groundvm, bit::crop<1>(data, 12));
	ASSERT_EQ(config.calib, bit::crop<1>(data, 13));
}

void verify_op(FGConfig const& config, std::bitset<32> const& data)
{
	ASSERT_EQ(config.maxcycle, bit::crop<8>(data, 0));
	ASSERT_EQ(config.readtime, bit::crop<6>(data, 8));
	ASSERT_EQ(config.acceleratorstep, bit::crop<6>(data, 14));
	ASSERT_EQ(config.voltagewritetime, bit::crop<6>(data, 20));
	ASSERT_EQ(config.currentwritetime, bit::crop<6>(data, 26));
}

TEST(FGBlock, Dimensions)
{
	// make sure not_connected has meaningfull value (at least something < 0)
	ASSERT_EQ(-1, not_connected);

	ASSERT_EQ(24, FGBlock::fg_lines);

	// check Coordinate
	for (size_t ii = 0; ii<4; ++ii)
	{
		FGBlockOnHICANN b{Enum{ii}};
		FGBlock block;

		if (b.toEnum() == 0 || b.toEnum() == 2) {
			ASSERT_EQ(halco::common::left, b.x());
			ASSERT_TRUE(block.is_left(b));
		} else {
			ASSERT_EQ(halco::common::right, b.x());
			ASSERT_FALSE(block.is_left(b));
		}
	}


	// check geometry and constants for shared fg values
	for (size_t ii = 0; ii<4; ++ii)
	{
		FGBlockOnHICANN b{Enum{ii}};
		FGBlock block;

		auto const& lut = block.getSharedLut(b);
		if (block.is_left(b)) {
			ASSERT_EQ(block.shared_lut_left, lut);
		} else {
			ASSERT_EQ(block.shared_lut_right, lut);
		}

		// analize histogram
		std::array<unsigned, 24> hist;
		for (auto& h : hist) h = 0;

		size_t cnt_nc = 0;
		for (auto const val : lut)
		{
			if (val == not_connected) {
				++cnt_nc;
			} else {
				hist.at(val) += 1;
			}
		}
		ASSERT_EQ(2, cnt_nc);
		for (auto const& h : hist)
			ASSERT_GE(1, h);
	}

	// check geometry and constants for neuron fg values
	for (size_t ii = 0; ii<4; ++ii)
	{
		FGBlockOnHICANN b{Enum{ii}};
		FGBlock block;

		auto const& lut = block.getNeuronLut(b);
		if (block.is_left(b)) {
			ASSERT_EQ(block.neuron_lut_left, lut);
		} else {
			ASSERT_EQ(block.neuron_lut_right, lut);
		}

		// analize histogram
		std::array<unsigned, 24> hist;
		for (auto& h : hist) h = 0;
		for (auto const val : lut)
			hist.at(val) += 1;
		for (auto const& h : hist)
			ASSERT_GE(1, h);
	}
}

TEST(FGBlock, Range)
{
	std::srand(time(NULL));

	size_t const iterations = 1000;
	for (size_t iter = 0; iter < iterations; ++iter)
	{
		FGBlockOnHICANN b{Enum{size_t(rand()) % 4}};
		FGBlock block;

		// make sure out of bound access throws

		// neuron access
		ASSERT_ANY_THROW(block.setNeuronRaw(rand() % 128, 24 + rand() % (INT_MAX/2), rand() % 1024));
		ASSERT_ANY_THROW(block.setNeuronRaw(128 + rand() % (INT_MAX/2), rand() % 24, rand() % 1024));
		ASSERT_ANY_THROW(block.getNeuronRaw(rand() % 128, 24 + rand() % (INT_MAX/2)));
		ASSERT_ANY_THROW(block.getNeuronRaw(128 + rand() % (INT_MAX/2), rand() % 24));
		ASSERT_ANY_THROW(block.setNeuron(b, NeuronOnFGBlock(Enum(rand() % 128)),
						 neuron_parameter(__last_neuron + rand() % (INT_MAX/2)), rand() % 1024));
		ASSERT_ANY_THROW(block.getNeuron(b, NeuronOnFGBlock(Enum(rand() % 128)),
						 neuron_parameter(__last_neuron + rand() % (INT_MAX/2))));

		// shared_access
		ASSERT_ANY_THROW(block.setSharedRaw(24 + rand() % (INT_MAX/2), rand() % 1024));
		ASSERT_ANY_THROW(block.getSharedRaw(24 + rand() % (INT_MAX/2)));
		ASSERT_ANY_THROW(block.setShared(b, shared_parameter(__last_shared + rand() % (INT_MAX/2)), rand() % 1024));
		ASSERT_ANY_THROW(block.getShared(b, shared_parameter(__last_shared + rand() % (INT_MAX/2))));
	}


	// test some invariants
	for (size_t ii = 0; ii<4; ++ii)
	{
		FGBlockOnHICANN b {Enum{ii}};
		FGBlock block;

		// test exclusive shared parameter
		auto const& lut = block.getSharedLut(b);
		if (block.is_left(b)) {
			ASSERT_EQ(block.shared_lut_left, lut);

			ASSERT_EQ(not_connected, lut.at(V_bexp));
			ASSERT_ANY_THROW(block.setShared(b, V_bexp, rand() % 1024));
			ASSERT_ANY_THROW(block.getShared(b, V_bexp));
			ASSERT_NO_THROW(block.setShared(b, V_bout, rand() % 1024));
			ASSERT_NO_THROW(block.getShared(b, V_bout));

			ASSERT_EQ(not_connected, lut.at(V_clrc));
			ASSERT_ANY_THROW(block.setShared(b, V_clrc, rand() % 1024));
			ASSERT_ANY_THROW(block.getShared(b, V_clrc));
			ASSERT_NO_THROW(block.setShared(b, V_clra, rand() % 1024));
			ASSERT_NO_THROW(block.getShared(b, V_clra));
		} else {
			ASSERT_EQ(block.shared_lut_right, lut);

			ASSERT_EQ(not_connected, lut.at(V_bout));
			ASSERT_NO_THROW(block.setShared(b, V_bexp, rand() % 1024));
			ASSERT_NO_THROW(block.getShared(b, V_bexp));
			ASSERT_ANY_THROW(block.setShared(b, V_bout, rand() % 1024));
			ASSERT_ANY_THROW(block.getShared(b, V_bout));

			ASSERT_EQ(not_connected, lut.at(V_clra));
			ASSERT_NO_THROW(block.setShared(b, V_clrc, rand() % 1024));
			ASSERT_NO_THROW(block.getShared(b, V_clrc));
			ASSERT_ANY_THROW(block.setShared(b, V_clra, rand() % 1024));
			ASSERT_ANY_THROW(block.getShared(b, V_clra));
		}
	}
}

TEST(FGBlock, RandomTest)
{
	std::srand(time(NULL));

	size_t const iterations = 1000;
	for (size_t iter = 0; iter < iterations; ++iter)
	{
		FGBlockOnHICANN b {Enum{size_t(rand()) % 4}};
		FGBlock block;

		randomize(block);

		// actuall verify formatted bits for all 24 FG parameters
		for (size_t ii = 0; ii<24; ++ii)
		{
			data_t data = block.set_formatter(b, ii);

			if (ii == 0) { // TODO: make sure this is correct
				verify_shared(block, data, ii);
			} else {
				verify_neuron(b, block, data, ii);
			}

			ASSERT_EQ(data, block.getFGRow(FGRowOnFGBlock(ii)).set_formatter());
		}

		// make sure out of bound access throws
		ASSERT_ANY_THROW(block.set_formatter(b, 24));
	}
}

TEST(FGBlock, Digital)
{
	size_t const iterations = 1000;
	for (size_t iter = 0; iter < iterations; ++iter)
	{
		FGConfig config;
		randomize(config);

		auto bias = config.getBias();
		verify_bias(config, bias);

		auto op = config.getOp();
		verify_op(config, op);
	}
}

TEST(FGBlock, RowConversion)
{
	// Left
	std::array<FGBlockOnHICANN, 2> left_blocks = {{
		FGBlockOnHICANN(X(0), Y(0)), FGBlockOnHICANN(X(0), Y(1))}};
	for (auto block : left_blocks)
	{
		EXPECT_EQ(I_convi,     getNeuronParameter(block, FGRowOnFGBlock(3)));
		EXPECT_EQ(I_gladapt,   getNeuronParameter(block, FGRowOnFGBlock(9)));
		EXPECT_EQ(I_gl,        getNeuronParameter(block, FGRowOnFGBlock(11)));
		EXPECT_EQ(V_exp,       getNeuronParameter(block, FGRowOnFGBlock(20)));
		EXPECT_EQ(int_op_bias, getSharedParameter(block, FGRowOnFGBlock(1)));
		EXPECT_EQ(V_bout,      getSharedParameter(block, FGRowOnFGBlock(3)));
		ASSERT_THROW(getSharedParameter(block, FGRowOnFGBlock(18)), std::out_of_range);
		EXPECT_EQ(V_clra,     getSharedParameter(block, FGRowOnFGBlock(12)));
		EXPECT_EQ(V_ccas,     getSharedParameter(block, FGRowOnFGBlock(23)));
	}

	// Right
	std::array<FGBlockOnHICANN, 2> right_blocks = {{
		FGBlockOnHICANN(X(1), Y(0)), FGBlockOnHICANN(X(1), Y(1))}};
	for (auto block : right_blocks)
	{
		EXPECT_EQ(I_convx,   getNeuronParameter(block, FGRowOnFGBlock(3)));
		EXPECT_EQ(I_pl,      getNeuronParameter(block, FGRowOnFGBlock(9)));
		EXPECT_EQ(I_gladapt, getNeuronParameter(block, FGRowOnFGBlock(11)));
		ASSERT_THROW(getNeuronParameter(block, FGRowOnFGBlock(22)), std::out_of_range);
		EXPECT_EQ(int_op_bias, getSharedParameter(block, FGRowOnFGBlock(1)));
		EXPECT_EQ(V_bexp,      getSharedParameter(block, FGRowOnFGBlock(3)));
		EXPECT_EQ(V_clrc,     getSharedParameter(block, FGRowOnFGBlock(12)));
		EXPECT_EQ(V_ccas,     getSharedParameter(block, FGRowOnFGBlock(23)));
	}
}

TEST(FGControl, Defaults)
{
	FGControl fgc;

	for (size_t ii=0; ii<4; ++ii)
	{
		FGBlockOnHICANN b {Enum{ii}};

		{
			// make sure FGblock is zero initialized
			FGBlock fgb;
			ASSERT_TRUE(std::all_of(fgb.mShared.begin(), fgb.mShared.end(),
				[](FGBlock::value_type const& v) { return v == 0; }));

			// and `setDefault` actually sets some values
			fgb.setDefault(b);
			ASSERT_TRUE(std::any_of(fgb.mShared.begin(), fgb.mShared.end(),
				[](FGBlock::value_type const& v) { return v != 0; }));

			// make sure FGBlockOnHICANN ctor works as well
			ASSERT_EQ(fgb, FGBlock(FGBlockOnHICANN(Enum(ii))));
		}


		// check default values, which are set in FGControl default ctor
		FGBlock const& fgb = fgc.getBlock(b);

		{ // shared
			typedef std::pair<shared_parameter, FGBlock::value_type> def_t;
			auto const& def = fgb.shared_default;
			for (size_t p=0; p<shared_parameter::__last_shared; ++p)
			{
				shared_parameter param = shared_parameter(p);

				auto it = std::find_if(def.begin(), def.end(),
					[param](def_t const& v){ return v.first == param; });

				ASSERT_NE(def.end(), it);
				try {
					ASSERT_EQ(it->second, fgb.getShared(b, param));
				} catch (...) {}
			}
		}

		{ // neuron
			typedef std::pair<neuron_parameter, FGBlock::value_type> def_t;
			auto const& def = fgb.neuron_default;
			for (size_t p=0; p<neuron_parameter::__last_neuron; ++p)
			{
				neuron_parameter param = neuron_parameter(p);

				auto it = std::find_if(def.begin(), def.end(),
					[param](def_t const& v){ return v.first == param; });

				ASSERT_NE(def.end(), it);

				for (size_t nrn=0; nrn<128; ++nrn)
				{
					try {
						ASSERT_EQ(it->second,
							fgb.getNeuron(b, NeuronOnFGBlock(nrn), param));
					} catch (...) {}
				}
			}
		}
	} // for all FGBlocks
}

} // HICANN
} // HMF
