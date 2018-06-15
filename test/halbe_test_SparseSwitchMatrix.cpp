#include <map>
#include <sstream>
#include <set>

#include <gtest/gtest.h>

#include "hal/backend/HMFBackend.h"
#include "hal/HICANN.h"
#include "hal/Coordinate/iter_all.h"

using namespace geometry;
using namespace ::HMF::Coordinate;

namespace HMF {
namespace HICANN {

TEST(Crossbar, Exists) {
	typedef Coordinate::HLineOnHICANN H;
	typedef Coordinate::VLineOnHICANN V;

	Crossbar crossbar;
	// left 
	for(size_t offset = 0; offset < 128; offset += 32) {
		EXPECT_TRUE(crossbar.exists(V(31 + offset), H( 0))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(31 + offset), H( 1))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(30 + offset), H( 2))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(30 + offset), H( 3))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(29 + offset), H( 4))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(29 + offset), H( 5))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 0 + offset), H(62))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 0 + offset), H(63))) << "offset = " << offset;
	}
	// right
	for(size_t offset = 128; offset < 256; offset += 32) {
		EXPECT_TRUE(crossbar.exists(V( 0 + offset), H( 0))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 0 + offset), H( 1))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 1 + offset), H( 2))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 1 + offset), H( 3))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 2 + offset), H( 4))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V( 2 + offset), H( 5))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(30 + offset), H(60))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(30 + offset), H(61))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(31 + offset), H(62))) << "offset = " << offset;
		EXPECT_TRUE(crossbar.exists(V(31 + offset), H(63))) << "offset = " << offset;
	}
	// Some random non existing things
	EXPECT_FALSE(crossbar.exists(V(  0), H( 0)));
	EXPECT_FALSE(crossbar.exists(V( 32), H(15)));
	EXPECT_FALSE(crossbar.exists(V(128), H(63)));
	EXPECT_FALSE(crossbar.exists(V(133), H(22)));

	// Sum for one row is 8:
	for (size_t y = 0; y < 64; ++y) {
		size_t sum_row = 0;
		for (size_t x = 0; x < 256; ++x) {
			sum_row += crossbar.exists(V(x), H(y));
		}
		EXPECT_EQ(8, sum_row);
	}

	// Throw or false:
	// Note: overflow_error is thrown when trying to construct coordinate (rant).
	try { EXPECT_FALSE( crossbar.exists(V(256), H(0))); }
	catch (...) { EXPECT_THROW( throw , std::overflow_error); }

	try { EXPECT_FALSE( crossbar.exists(V(257), H(0))); }
	catch (...) { EXPECT_THROW( throw , std::overflow_error); }

	try { EXPECT_FALSE( crossbar.exists(V(256 + 32), H(0))); }
	catch (...) { EXPECT_THROW( throw , std::overflow_error); }

	try { EXPECT_FALSE( crossbar.exists(V(0), H(127))); }
	catch (...) { EXPECT_THROW( throw , std::overflow_error); }
}

TEST(Crossbar, IsEmptyInitially) {
	typedef Coordinate::HLineOnHICANN H;
	typedef Coordinate::VLineOnHICANN V;

	Crossbar crossbar;
	for ( size_t x=0; x < Crossbar::x_type::end; ++x) {
		for ( size_t y=0; y < Crossbar::y_type::end; ++y) {
			if (crossbar.exists(V(x), H(y))) {
				EXPECT_FALSE(crossbar.get(V(x), H(y)));
			}
		}
	}
}

TEST(Crossbar, GetSet) {
	typedef Coordinate::HLineOnHICANN H;
	typedef Coordinate::VLineOnHICANN V;

	Crossbar crossbar;
	// set and get
	crossbar.set(V(0), H(62), true);
	EXPECT_TRUE(crossbar.get(V(0), H(62)));

	// set and check internal
	crossbar.set(V(63), H(0), true);
	EXPECT_TRUE(crossbar.switches()[0][1]);

	// throw on non existing switch
	// Note: overflow_error is thrown when trying to construct coordinate (rant).
	// getter
	EXPECT_ANY_THROW( crossbar.get(V(0), H(0)));
	EXPECT_THROW( crossbar.get(V(555), H(0)), std::overflow_error);
	EXPECT_THROW( crossbar.get(V(0), H(66)), std::overflow_error);

	// setter
	EXPECT_ANY_THROW(std::ignore = (crossbar.get(V(0), H(0)) == true));
	EXPECT_THROW(std::ignore = (crossbar.get(V(555), H(0)) == true), std::overflow_error);
	EXPECT_THROW(std::ignore = (crossbar.get(V(0), H(66)) == true), std::overflow_error);
}

TEST(Crossbar, GetRowConfig) {
	typedef Coordinate::HLineOnHICANN H;
	typedef Coordinate::VLineOnHICANN V;

	Crossbar crossbar;
	crossbar.set(V(0), H(62), true);

	// left
	{
		auto row_config = crossbar.get_row(Coordinate::HLineOnHICANN(62), left);
		EXPECT_EQ(4, row_config.size());
		EXPECT_TRUE(row_config[0]);
		EXPECT_FALSE(row_config[1]);
		EXPECT_FALSE(row_config[2]);
		EXPECT_FALSE(row_config[3]);
	}

	// right
	crossbar.set(V(128+32), H(0), true);
	{
		auto row_config = crossbar.get_row(Coordinate::HLineOnHICANN(0), right);
		EXPECT_EQ(4, row_config.size());
		EXPECT_FALSE(row_config[0]);
		EXPECT_TRUE(row_config[1]);
		EXPECT_FALSE(row_config[2]);
		EXPECT_FALSE(row_config[3]);
	}
}

TEST(Crossbar, CheckExclusiveness) {
	typedef Coordinate::HLineOnHICANN H;
	typedef Coordinate::VLineOnHICANN V;

	Crossbar c1, c2;

	// empty
	EXPECT_TRUE("" == c1.check_exclusiveness(1, 1));

	// row-wise
	//
	// enable 1 switch. should be ok
	c1.set(V(0), H(62), true);
	EXPECT_TRUE("" == c1.check_exclusiveness(1, 1));

	// enable 2nd switch in same row -> should throw.
	c1.set(V(32), H(62), true);
	EXPECT_FALSE("" == c1.check_exclusiveness(1, 1));

	// column-wise
	// enable 1 switch. should be ok
	c2.set(V(0), H(62), true);
	EXPECT_TRUE("" == c2.check_exclusiveness(1, 1));

	// enable 2nd switch in same colum-> should throw.
	c2.set(V(0), H(63), true);
	EXPECT_FALSE("" == c2.check_exclusiveness(1, 1));
}

TEST(Crossbar, GetLines)
{
	using namespace HMF::Coordinate;

	for (unsigned ii = 0; ii < HLineOnHICANN::end; ++ii)
	{
		HLineOnHICANN hline(ii);
		EXPECT_NO_THROW(Crossbar::get_lines(hline));
	}

	typedef std::set<unsigned> us_t;

	std::map<HLineOnHICANN, us_t> hlines;
	us_t us;

	us = us_t{ 10, 42, 74, 106, 149, 181, 213, 245 };
	hlines.insert(std::make_pair(HLineOnHICANN(42), us));
	us = us_t{ 28, 60, 92, 124, 131, 163, 195, 227 };
	hlines.insert(std::make_pair(HLineOnHICANN(6), us));

	for (auto p : hlines) {
		std::set<VLineOnHICANN> diff, expected;

		for (auto ii : p.second) {
			expected.insert(VLineOnHICANN(ii));
		}

		auto vlines = Crossbar::get_lines(p.first);
		std::set_symmetric_difference(vlines.begin(), vlines.end(),
			expected.begin(), expected.end(),
			std::inserter(diff, diff.begin()));

		EXPECT_TRUE(diff.empty());
	}
}

TEST(Crossbar, StreamOperator)
{
	std::ostringstream os;
	Crossbar cb;
	os << cb;
}

TEST(Crossbar, EqualOperator)
{
	typedef Crossbar type;
	type a, b;
	ASSERT_TRUE(a == b);
	ASSERT_FALSE(a != b);


	for (size_t xx=0; xx<256; ++xx)
	{
		if (a.exists(type::x_type(xx), type::y_type(0))) {
			a.set(type::x_type(xx), type::y_type(0), true);
			break;
		}
	}

	ASSERT_TRUE(a != b);
	ASSERT_FALSE(a == b);
}

TEST(SynapseSwitch, Exists) {
	typedef Coordinate::VLineOnHICANN V;
	typedef Coordinate::SynapseSwitchRowOnHICANN::y_type H;

	SynapseSwitch s;

	// top left
	for(size_t row = 0; row < 112; row+=2) {
		size_t local_offset = ((row/2)*4)%32;

		for(size_t block_offset = 0; block_offset < 128; block_offset+=32) {
			// 4x2 block
			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(0+row)));

			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(1+row)));
		}
	}

	// top right 
	for(size_t row = 0; row < 112; row+=2) {
		// local offest: 28 for row 0
		// local offest: 24 for row 2
		// local offest: 20 for row 4
		// local offest: 0 for row 14
		size_t local_offset = (28 - ((row/2)*4))%32;

		for(size_t block_offset = 128; block_offset < 256; block_offset+=32) {
			// 4x2 block
			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(0+row)));

			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(1+row)));
		}
	}

	// bottom left
	for(size_t row = 112; row < 224; row+=2) {
		// local offest: 28 for row 0
		// local offest: 24 for row 2
		// local offest: 20 for row 4
		// local offest: 0 for row 14
		size_t local_offset = (28 - ((row/2)*4))%32;

		for(size_t block_offset = 0; block_offset < 128; block_offset+=32) {
			// 4x2 block
			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(0+row)));

			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(1+row)));
		}
	}

	// bottom right
	for(size_t row = 112; row < 224; row+=2) {
		size_t local_offset = ((row/2)*4)%32;

		for(size_t block_offset = 128; block_offset < 256; block_offset+=32) {
			// 4x2 block
			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(0+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(0+row)));

			EXPECT_TRUE(s.exists(V(0 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(1 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(2 + block_offset + local_offset), H(1+row)));
			EXPECT_TRUE(s.exists(V(3 + block_offset + local_offset), H(1+row)));
		}
	}

	// CHECK some for false:
	EXPECT_FALSE(s.exists(V(  0), H(  3)));
	EXPECT_FALSE(s.exists(V( 20), H(  5)));
	EXPECT_FALSE(s.exists(V(  0), H(128)));
	EXPECT_FALSE(s.exists(V( 40), H(128)));
	EXPECT_FALSE(s.exists(V(138), H(  9)));
	EXPECT_FALSE(s.exists(V(255), H(111)));
	EXPECT_FALSE(s.exists(V(250), H(112)));

	// Sum for one row is 32:
	for (size_t y = 0; y < 224; ++y) {
		size_t sum_row = 0;
		for (size_t x = 0; x < 256; ++x) {
			sum_row += s.exists(V(x), H(y));
		}
		EXPECT_EQ(32, sum_row);
	}

	// TODO: out of range check
}

TEST(SynapseSwitch, IsEmptyInitially) {
	typedef Coordinate::VLineOnHICANN V;
	typedef Coordinate::SynapseSwitchRowOnHICANN::y_type H;

	SynapseSwitch s;
	for ( size_t x=0; x < V::end; ++x) {
		for ( size_t y=0; y < H::end; ++y) {
			if (s.exists(V(x), H(y))) {
				EXPECT_FALSE(s.get(V(x), H(y)));
			}
		}
	}
}

TEST(SynapseSwitch, GetSet) {
	typedef Coordinate::VLineOnHICANN V;
	typedef Coordinate::SynapseSwitchRowOnHICANN::y_type H;

	SynapseSwitch s;
	// TODO

	// top left
	s.set(V(3), H(0), true);
	EXPECT_TRUE( s.get(V(3), H(0)));
	EXPECT_TRUE( s.switches()[0][3] );

	// bottom left
	s.set(V(62), H(112), true);
	EXPECT_TRUE( s.get(V(62), H(112)));
	EXPECT_TRUE( s.switches()[112][6] );

	// top right
	s.set(V(137), H(11), true);
	EXPECT_TRUE( s.get(V(137), H(11)));
	EXPECT_TRUE( s.switches()[11][17] );

	// bottom right
	s.set(V(128 + 2*32 + 16 + 2), H(112+24), true);
	EXPECT_TRUE( s.get(V(128 + 2*32 + 16 + 2), H(112 + 24)));
	EXPECT_TRUE( s.switches()[112 + 24 ][26] );
}

TEST(SynapseSwitch, GetRowConfig) {
	typedef Coordinate::VLineOnHICANN V;
	typedef Coordinate::SynapseSwitchRowOnHICANN::y_type H;

	SynapseSwitch s;
	// top left
	s.set(V(96), H(0), true);
	{
		size_t pos_true = 12;
		auto row_config = s.get_row(Coordinate::SynapseSwitchRowOnHICANN(Y(0),left));
		EXPECT_EQ(16, row_config.size());
		for (size_t pos = 0; pos < row_config.size(); ++pos) {
			if (pos == pos_true)
				EXPECT_TRUE( row_config[pos] );
			else
				EXPECT_FALSE( row_config[pos] );
		}
	}

	// bottom left
	s.set(V(54), H(116), true);
	{
		size_t pos_true = 6;
		auto row_config = s.get_row(Coordinate::SynapseSwitchRowOnHICANN(Y(116),left));
		EXPECT_EQ(16, row_config.size());
		for (size_t pos = 0; pos < row_config.size(); ++pos) {
			if (pos == pos_true)
				EXPECT_TRUE( row_config[pos] );
			else
				EXPECT_FALSE( row_config[pos] );
		}
	}

	// top right
	s.set(V(137), H(11), true);
	{
		size_t pos_true = 1;
		auto row_config = s.get_row(Coordinate::SynapseSwitchRowOnHICANN(Y(11),right));
		EXPECT_EQ(16, row_config.size());
		for (size_t pos = 0; pos < row_config.size(); ++pos) {
			if (pos == pos_true)
				EXPECT_TRUE( row_config[pos] );
			else
				EXPECT_FALSE( row_config[pos] );
		}
	}

	// bottom right
	s.set(V(128 + 2*32 + 16 + 2), H(112+24), true);
	{
		size_t pos_true = 10;
		auto row_config = s.get_row(Coordinate::SynapseSwitchRowOnHICANN(Y(112+24),right));
		EXPECT_EQ(16, row_config.size());
		for (size_t pos = 0; pos < row_config.size(); ++pos) {
			if (pos == pos_true)
				EXPECT_TRUE( row_config[pos] );
			else
				EXPECT_FALSE( row_config[pos] );
		}
	}
}

TEST(SynapseSwitch, CheckExclusiveness) {
	typedef Coordinate::VLineOnHICANN V;
	typedef Coordinate::SynapseSwitchRowOnHICANN::y_type H;

	SynapseSwitch s1, s2;

	EXPECT_TRUE("" == s1.check_exclusiveness(1, 1));

	// enable one switch
	s1.set(V(32), H(1), true);
	EXPECT_TRUE("" == s1.check_exclusiveness(1, 1));

	// another one in same row, but now on the right side
	s1.clear();
	s1.set(V(32), H(0), true);
	s1.set(V(255), H(0), true);
	EXPECT_TRUE("" == s1.check_exclusiveness(1, 1));

	// two on the left side
	s1.clear();
	s1.set(V(32), H(0), true);
	s1.set(V(32), H(16), true);
	EXPECT_FALSE("" == s1.check_exclusiveness(1, 1));
	EXPECT_TRUE("" == s1.check_exclusiveness(1, 2));

	// two in the same row on the same side
	s1.clear();
	s1.set(V(32), H(16), true);
	s1.set(V(33), H(16), true);
	EXPECT_FALSE("" == s1.check_exclusiveness(1, 1));
	EXPECT_TRUE("" == s1.check_exclusiveness(2, 1));

	// check exclusiveness with right neighbour
	EXPECT_NO_THROW(s1.check_exclusiveness(s2));
	// enable one on the right side of left hicann
	s1.set(V(128 + 32), H(112 + 16), true);
	EXPECT_NO_THROW(s1.check_exclusiveness(s2));
	// enable one on the left side of right hicann(same row)
	s2.set(V(30), H(112 + 16), true);
	EXPECT_ANY_THROW(s1.check_exclusiveness(s2));
}

TEST(SynapseSwitch, GetLines)
{
	using namespace HMF::Coordinate;
	typedef std::set<unsigned> us_t;

	std::map<SynapseSwitchRowOnHICANN::y_type, us_t> slines;
	us_t us;

	us = us_t{ 28, 60, 92, 124, 128, 160, 192, 224};
	slines.insert(std::make_pair(SynapseSwitchRowOnHICANN(Y(111), left).line(), us));

	for (auto p : slines) {
		std::set<VLineOnHICANN> diff, expected;

		for (auto ii : p.second) {
			for (size_t jj = 4; jj--; ) {
				expected.insert(VLineOnHICANN(ii + jj));
			};
		}

		auto vlines = SynapseSwitch::get_lines(p.first);
		std::set_symmetric_difference(vlines.begin(), vlines.end(),
			expected.begin(), expected.end(),
			std::inserter(diff, diff.begin()));
		EXPECT_TRUE(diff.empty());
	}
}

TEST(SynapseSwitch, GetLinesFromRows)
{
	using namespace HMF::Coordinate;
	typedef std::set<unsigned> us_t;

	std::map<SynapseSwitchRowOnHICANN, us_t> slines;
	us_t us;

	us = us_t{ 28, 60, 92, 124 };
	slines.insert(std::make_pair(SynapseSwitchRowOnHICANN(Y(111), left), us));

	us = us_t{ 128, 160, 192, 224};
	slines.insert(std::make_pair(SynapseSwitchRowOnHICANN(Y(111), right), us));

	for (auto p : slines) {
		std::set<VLineOnHICANN> diff, expected;

		for (auto ii : p.second) {
			for (size_t jj = 4; jj--; ) {
				expected.insert(VLineOnHICANN(ii + jj));
			};
		}

		auto vlines = SynapseSwitch::get_lines(p.first);
		std::set_symmetric_difference(vlines.begin(), vlines.end(),
			expected.begin(), expected.end(),
			std::inserter(diff, diff.begin()));
		EXPECT_TRUE(diff.empty());
	}
}

TEST(SynapseSwitch, StreamOperator)
{
	std::ostringstream os;
	SynapseSwitch syn;
	os << syn;
}

TEST(SynapseSwitch , EqualOperator)
{
	typedef SynapseSwitch type;
	type a, b;
	ASSERT_TRUE(a == b);
	ASSERT_FALSE(a != b);

	for (size_t xx=0; xx<256; ++xx)
	{
		if (a.exists(type::x_type(xx), type::y_type(0))) {
			a.set(type::x_type(xx), type::y_type(0), true);
			break;
		}
	}

	ASSERT_TRUE(a != b);
	ASSERT_FALSE(a == b);
}

TEST(SynapseSwitch, CompareToSynapseSwitchOnHICANN)
{
	for (auto syn_switch : iter_all<SynapseSwitchOnHICANN>())
	{
		VLineOnHICANN line(syn_switch.x());
		ASSERT_TRUE(SynapseSwitch::exists(line, Y(syn_switch.y())))
			<< line << ":" << syn_switch;
		ASSERT_TRUE(SynapseSwitchOnHICANN::exists(syn_switch.x(), syn_switch.y()));
	}
}

} // namespace HICANN
} // namespace HMF
