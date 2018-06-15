#include <gtest/gtest.h>

#include "hal/backend/HICANNBackendHelper.h"
#include "test/NeuronBuilderHelper.h"


using namespace HMF::HICANN;
using namespace HMF::Coordinate;


namespace HMF {

TEST(NeuronBuilder, RandomTest)
{
	// make sure neuron 1 is top right neuron
	ASSERT_EQ(NeuronOnQuad(Enum(1)), NeuronOnQuad(X(1), Y(0)));

	std::srand(time(NULL));

	size_t const iterations = 100000;

	for (size_t ii = 0; ii < iterations; ++ii)
	{
		NeuronQuad quad = randomize_quad();

		// iomapping throws for invalid combinations
		bool should_throw = false;
		size_t cnt = 0;
		std::array<std::bitset<4>, 2> io;
		for (auto& c : io)
		{
			c = getIObits(Y(cnt++), quad);

			size_t t = c.to_ulong();
			ASSERT_LT(t, 16);
			if (iomap.at(t) == -1) {
				should_throw = true;
				ASSERT_ANY_THROW(denmem_iomap(c));
				ASSERT_ANY_THROW(format(quad));
			} else {
				ASSERT_NO_THROW(denmem_iomap(c));
			}
		}

		// skip if we have an invalid iomap configuration
		if (should_throw)
			continue;

		// actual call to formatter
		auto data = format(quad);

		// verify results
		for (size_t jj=0; jj<4; ++jj)
			verify(quad, DenmemConfig(jj, data[jj]));

		// verify the "readback from hardware" reverse formatting
		verify_reader(quad, data);
	}
}

TEST(NeuronBuilder, NeuronAddressMapping)
{
	std::array<int, 4> const map = {{ 1, 3, 0, 2, }};
	for (size_t ii=0; ii<4; ++ii)
		ASSERT_EQ(map[ii], NeuronQuad::getHWAddress(NeuronOnQuad(Enum(ii))));
}

} // HMF
