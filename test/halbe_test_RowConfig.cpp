#include <gtest/gtest.h>

#include "hal/HICANN/RowConfig.h"

using namespace ::HMF::HICANN;
using namespace halco::common;

TEST(RowConfig, GmaxDiv)
{
	RowConfig rowConfig;

	// test corner cases
	rowConfig.set_gmax_div(GmaxDiv(0));
	ASSERT_EQ(rowConfig.get_gmax_div(left), 0);
	ASSERT_EQ(rowConfig.get_gmax_div(right), 0);

	rowConfig.set_gmax_div(GmaxDiv(1));
	ASSERT_EQ(rowConfig.get_gmax_div(left), 1);
	ASSERT_EQ(rowConfig.get_gmax_div(right), 0);

	rowConfig.set_gmax_div(GmaxDiv(16));
	ASSERT_EQ(rowConfig.get_gmax_div(left), 15);
	ASSERT_EQ(rowConfig.get_gmax_div(right), 1);

	rowConfig.set_gmax_div(GmaxDiv(30));
	ASSERT_EQ(rowConfig.get_gmax_div(left), 15);
	ASSERT_EQ(rowConfig.get_gmax_div(right), 15);
}
