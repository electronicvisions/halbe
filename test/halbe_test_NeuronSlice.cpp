#include <gtest/gtest.h>

#include "hal/HICANN.h"

using namespace std;
using namespace geometry;

namespace HMF {

class NeuronQuadTest : public ::testing::Test {
protected:
	virtual void SetUp() { // called for every TEST_F
		nquad = HICANN::NeuronQuad();
	}
	HICANN::NeuronQuad nquad;
};

TEST_F(NeuronQuadTest, SizeCheck) {
	EXPECT_EQ(nquad.width(), 2);
	EXPECT_EQ(nquad.height(), 2);
}

TEST_F(NeuronQuadTest, NeuronAccess) {
	HICANN::Neuron nrn;
	nrn.address(HICANN::L1Address(22));

	nquad[Coordinate::NeuronOnQuad(X(0),Y(0))] = nrn;
	EXPECT_EQ( (nquad[Coordinate::NeuronOnQuad(X(0),Y(0))]), nrn);

	// Note: Error is thrown when trying to construct coordinate (rant).
	ASSERT_THROW(Coordinate::NeuronOnQuad::x_type x(2), overflow_error);
	ASSERT_THROW(Coordinate::NeuronOnQuad::x_type x(3), overflow_error);
	ASSERT_THROW(Coordinate::NeuronOnQuad::y_type x(2), overflow_error);
	ASSERT_THROW(Coordinate::NeuronOnQuad::y_type x(3), overflow_error);


	// setter
	ASSERT_THROW( (nquad[Coordinate::NeuronOnQuad(X(2),Y(0))] = nrn), overflow_error);
	ASSERT_THROW( (nquad[Coordinate::NeuronOnQuad(X(0),Y(15))] = nrn), overflow_error);

	// getter
	ASSERT_THROW( (nrn = nquad[Coordinate::NeuronOnQuad(X(2), Y(0))]), overflow_error);
	ASSERT_THROW( (nrn = nquad[Coordinate::NeuronOnQuad(X(0),Y(15))]), overflow_error);
}

TEST_F(NeuronQuadTest, InterConnectVertical) {
	bool cv0 = true;
	nquad.setVerticalInterconnect(X(0), cv0);
	EXPECT_EQ( nquad.getVerticalInterconnect(X(0)), cv0);

	bool cv1 = true;
	nquad.setVerticalInterconnect(X(1), cv1);
	EXPECT_EQ( nquad.getVerticalInterconnect(X(1)), cv1);

	// Note: Error is thrown when trying to construct coordinate (see above).
	// setter
	ASSERT_THROW( nquad.setVerticalInterconnect(X(2), true), overflow_error);
	// getter
	ASSERT_THROW( nquad.getVerticalInterconnect(X(3)), overflow_error);
}

TEST_F(NeuronQuadTest, InterConnectHorizontal) {
	bool ch0 = true;
	nquad.setHorizontalInterconnect(Y(0), ch0);
	EXPECT_EQ( nquad.getHorizontalInterconnect(Y(0)), ch0);

	bool ch1 = true;
	nquad.setHorizontalInterconnect(Y(1), ch1);
	EXPECT_EQ( nquad.getHorizontalInterconnect(Y(1)), ch1);

	// Note: Error is thrown when trying to construct coordinate (see above).
	// setter
	ASSERT_THROW( nquad.setHorizontalInterconnect(Y(2), true), overflow_error);
	// getter
	ASSERT_THROW( nquad.getHorizontalInterconnect(Y(3)), overflow_error);
}

} // namespace HMF
