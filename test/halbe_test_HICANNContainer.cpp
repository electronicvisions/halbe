#include <gtest/gtest.h>
#include <fstream>
#include <ostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include "hal/HICANNContainer.h"

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace HMF {

template<typename T>
class RangedTest : public ::testing::Test
{};

typedef ::testing::Types<
	HICANN::L1Address,
	SynapseWeight,
	SynapseDecoder,
	DriverDecoder,
	SynapseSel,
	SynapseGen,
	SynapseDllresetb
	// TODO add more
> RangedTypes;

TYPED_TEST_CASE(RangedTest, RangedTypes);

TYPED_TEST(RangedTest, isAssignable)
{
	TypeParam a, b;
	a = b;
	ASSERT_EQ(a, b);
}

TYPED_TEST(RangedTest, isCopyable)
{
	TypeParam a;
	TypeParam b(a);
	ASSERT_EQ(a, b);
}

TYPED_TEST(RangedTest, IsPrintable) // uppercase as in test-Coordinate.cpp
{
	TypeParam obj;
	std::stringstream tmp;
	tmp << obj;
	ASSERT_FALSE(tmp.str().empty());
}

TYPED_TEST(RangedTest, RangedAssertions)
{
	typedef TypeParam type;
	typedef typename type::value_type value_type;
	ASSERT_EQ(type::end, type::max+1);

	// corner cases
	ASSERT_NO_THROW(type{type::min});
	ASSERT_NO_THROW(type{type::max});

	for (value_type ii=type::min; ii<type::end; ++ii)
		ASSERT_NO_THROW(type{ii});

	ASSERT_ANY_THROW(type{value_type(type::min-1)});
	ASSERT_ANY_THROW(type{type::end});
}

TEST(Synapses, WeightConversion)
{
	std::bitset<4> v(3);
	SynapseWeight  w = SynapseWeight::from_bitset(v);
	std::bitset<4> t(w);
	ASSERT_EQ(v, t);
}

TEST(Synapses, SynapseDecoderConversion)
{
	std::bitset<4> v(3);
	SynapseDecoder w = SynapseDecoder::from_bitset(v);
	std::bitset<4> t(w);
	ASSERT_EQ(v, t);
}

TEST(Synapses, DriverDecoderConversion)
{
	std::bitset<2> v(3);
	DriverDecoder w = DriverDecoder::from_bitset(v);
	std::bitset<2> t(w);
	ASSERT_EQ(v, t);
}

TEST(NeuronConfig, NeuronConfigSerialization)
{
	auto const basedir = getenv("BASEDIR_NMPM_SOFTWARE");
	if (basedir == nullptr) {
		throw std::runtime_error("BASEDIR_NMPM_SOFTWARE is not set");
	}

	namespace fs = boost::filesystem;
	std::ifstream file_v1{
	    (fs::path(basedir) / fs::path("share/halbe/NeuronConfig_v1.txt")).string()};
	boost::archive::text_iarchive ia_v1{file_v1};
	NeuronConfig nrn_cfg_v1;
	ia_v1 >> nrn_cfg_v1;
	ASSERT_EQ(nrn_cfg_v1.get_neuron_reset(), true);
	ASSERT_EQ(nrn_cfg_v1.get_spl1_reset(), true);

	std::ifstream file_v2{
	    (fs::path(basedir) / fs::path("share/halbe/NeuronConfig_v2.txt")).string()};
	boost::archive::text_iarchive ia_v2{file_v2};
	NeuronConfig nrn_cfg_v2;
	ia_v2 >> nrn_cfg_v2;
	ASSERT_EQ(nrn_cfg_v2.get_neuron_reset(), true);
	ASSERT_EQ(nrn_cfg_v2.get_spl1_reset(), true);
	ASSERT_EQ(nrn_cfg_v2.timings, HMF::HICANN::SRAMControllerTimings(
	    HMF::HICANN::SRAMReadDelay(1), HMF::HICANN::SRAMSetupPrecharge(2),
	    HMF::HICANN::SRAMWriteDelay(3)));
}

TEST(RepeaterBlock, RepeaterBlockSerialization)
{
	auto const basedir = getenv("BASEDIR_NMPM_SOFTWARE");
	if (basedir == nullptr) {
		throw std::runtime_error("BASEDIR_NMPM_SOFTWARE is not set");
	}

	namespace fs = boost::filesystem;
	std::ifstream file_v1{
	    (fs::path(basedir) / fs::path("share/halbe/RepeaterBlock_v1.txt")).string()};
	boost::archive::text_iarchive ia_v1{file_v1};
	RepeaterBlock repeater_block_v1;
	ia_v1 >> repeater_block_v1;
	ASSERT_EQ(repeater_block_v1.timings, HMF::HICANN::SRAMControllerTimings(
	    HMF::HICANN::SRAMReadDelay(3), HMF::HICANN::SRAMSetupPrecharge(2),
	    HMF::HICANN::SRAMWriteDelay(1)));
}

TEST(SynapseControlRegister, ConstructionRowAndLastRow)
{
	// Assert that constructor initilaises row to the first row of the corresponding
	// synapse array and last_row to the last
	typedef Coordinate::SynapseRowOnHICANN row_t;
	Coordinate::SynapseArrayOnHICANN top_array(0);
	Coordinate::SynapseArrayOnHICANN bottom_array(1);
	HMF::HICANN::SynapseControlRegister ctrl_top(top_array);
	HMF::HICANN::SynapseControlRegister ctrl_bottom(bottom_array);

	ASSERT_EQ(ctrl_top.get_row(), row_t(0));
	ASSERT_EQ(ctrl_top.get_last_row(), row_t(223));

	ASSERT_EQ(ctrl_bottom.get_row(), row_t(224));
	ASSERT_EQ(ctrl_bottom.get_last_row(), row_t(447));
}

TEST(SynapseControlRegister, SetGetRow)
{
	typedef Coordinate::SynapseRowOnHICANN syn_row_t;
	HMF::HICANN::SynapseControlRegister ctrl_reg_top;
	HMF::HICANN::SynapseControlRegister ctrl_reg_bottom(Coordinate::SynapseArrayOnHICANN(1));

	syn_row_t row(4);
	ctrl_reg_top.set_row(row);
	ASSERT_EQ(ctrl_reg_top.get_row(), row);

	// last row may not exceed maximal value for top array
	row = syn_row_t(224);
	ASSERT_ANY_THROW(ctrl_reg_top.set_row(row));

	// possible to set and get maximal values
	row = syn_row_t(223);
	ctrl_reg_top.set_row(row);
	ASSERT_EQ(ctrl_reg_top.get_row(), row);
	row = syn_row_t(syn_row_t::max);
	ctrl_reg_bottom.set_row(row);
	ASSERT_EQ(ctrl_reg_bottom.get_row(), row);

	// row has to be on specified synapse array
	row = syn_row_t(1);
	ASSERT_ANY_THROW(ctrl_reg_bottom.set_row(row));
}

TEST(SynapseControlRegister, SetGetLastRow)
{
	typedef Coordinate::SynapseRowOnHICANN syn_row_t;
	HMF::HICANN::SynapseControlRegister ctrl_reg_top;
	HMF::HICANN::SynapseControlRegister ctrl_reg_bottom(Coordinate::SynapseArrayOnHICANN(1));

	syn_row_t row(4);
	ctrl_reg_top.set_last_row(row);
	ASSERT_EQ(ctrl_reg_top.get_last_row(), row);

	// last row may not exceed maximal value for top array
	row = syn_row_t(224);
	ASSERT_ANY_THROW(ctrl_reg_top.set_last_row(row));

	// possible to set and get maximal values
	row = syn_row_t(223);
	ctrl_reg_top.set_last_row(row);
	ASSERT_EQ(ctrl_reg_top.get_last_row(), row);
	row = syn_row_t(syn_row_t::max);
	ctrl_reg_bottom.set_last_row(row);
	ASSERT_EQ(ctrl_reg_bottom.get_last_row(), row);

	// row has to be on specified synapse array
	row = syn_row_t(1);
	ASSERT_ANY_THROW(ctrl_reg_bottom.set_last_row(row));
}

TEST(SynapseControlRegister, Comparison)
{
	HMF::HICANN::SynapseControlRegister ctrl_reg1;
	HMF::HICANN::SynapseControlRegister ctrl_reg2;
	HMF::HICANN::SynapseControlRegister ctrl_reg3;
	ctrl_reg3.newcmd = true;

	// comparison with two undefined values (idle of ctrl_regx)
	ASSERT_TRUE(ctrl_reg1 == ctrl_reg2);
	ASSERT_FALSE(ctrl_reg1 == ctrl_reg3);

	// comparison with one undefined value (idle of ctrl_reg1)
	ctrl_reg2.idle = true;
	ctrl_reg3.idle = true;
	ASSERT_TRUE(ctrl_reg1 == ctrl_reg2);
	ASSERT_FALSE(ctrl_reg1 == ctrl_reg3);

	// comparison with all values defined
	ctrl_reg1.idle = true;
	ASSERT_TRUE(ctrl_reg1 == ctrl_reg2);
	ASSERT_FALSE(ctrl_reg1 == ctrl_reg3);

	ctrl_reg1.idle = false;
	ASSERT_FALSE(ctrl_reg1 == ctrl_reg2);
	ASSERT_FALSE(ctrl_reg1 == ctrl_reg3);
}

} // HMF
