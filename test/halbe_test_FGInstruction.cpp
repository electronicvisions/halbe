#include <gtest/gtest.h>
#include <cstdlib>
#include <bitter/bitter.h>

#include "hal/HICANN/FGInstruction.h"

namespace HMF {
namespace HICANN {

std::bitset<17>
strip(FGInstruction const& i)
{
	return std::bitset<17>(i);
}

std::bitset<3>
cmd(FGInstruction const& i)
{
	return bit::crop<3>(strip(i), 14);
}

std::bitset<1>
bank(FGInstruction const& i)
{
	return bit::crop<1>(strip(i), 13);
}

std::bitset<5>
row(FGInstruction const& i)
{
	return bit::crop<5>(strip(i), 0);
}

std::bitset<8>
col(FGInstruction const& i)
{
	return bit::crop<8>(strip(i), 5);
}

TEST(FGInstruction, Default)
{
	ASSERT_EQ(0, strip(FGInstruction()).to_ulong());
}

TEST(FGInstruction, getNextFalse)
{
	FGInstruction instr = FGInstruction::getNextFalse();

	ASSERT_EQ(FGInstruction::_getNextFalse,
			  cmd(instr).to_ulong());

	ASSERT_EQ(0, row(instr).to_ulong());
	ASSERT_EQ(0, col(instr).to_ulong());
	ASSERT_EQ(0, bank(instr).to_ulong());
}

TEST(FGInstruction, writeDown)
{
	ASSERT_ANY_THROW(FGInstruction::writeDown(24));

	size_t const iter = 1000;
	for (size_t ii=0; ii<iter; ++ii)
	{
		size_t const r = rand() % 24;
		bool const b = rand() % 2;
		FGInstruction instr = FGInstruction::writeDown(r, b);

		ASSERT_EQ(FGInstruction::_writeDown, cmd(instr).to_ulong());
		ASSERT_EQ(r, row(instr).to_ulong());
		ASSERT_EQ(b, bank(instr).to_ulong());
		ASSERT_EQ(0, col(instr).to_ulong());
	}
}

TEST(FGInstruction, writeUp)
{
	ASSERT_ANY_THROW(FGInstruction::writeUp(24));

	size_t const iter = 1000;
	for (size_t ii=0; ii<iter; ++ii)
	{
		size_t const r = rand() % 24;
		bool const b = rand() % 2;
		FGInstruction instr = FGInstruction::writeUp(r, b);

		ASSERT_EQ(FGInstruction::_writeUp, cmd(instr).to_ulong());
		ASSERT_EQ(r, row(instr).to_ulong());
		ASSERT_EQ(b, bank(instr).to_ulong());
		ASSERT_EQ(0, col(instr).to_ulong());
	}
}

TEST(FGInstruction, read)
{
	ASSERT_ANY_THROW(FGInstruction::read(24, 0));
	ASSERT_ANY_THROW(FGInstruction::read(0, 129));

	size_t const iter = 1000;
	for (size_t ii=0; ii<iter; ++ii)
	{
		size_t const r = rand() % 24;
		size_t const c = rand() % 120;
		bool const b = rand() % 2;
		FGInstruction instr = FGInstruction::read(r, c, b);

		ASSERT_EQ(FGInstruction::_read, cmd(instr).to_ulong());
		ASSERT_EQ(r, row(instr).to_ulong());
		ASSERT_EQ(b, bank(instr).to_ulong());
		ASSERT_EQ(c, col(instr).to_ulong());
	}
}

TEST(FGInstruction, stimulateNeurons)
{
	size_t const iter = 1000;
	for (size_t ii=0; ii<iter; ++ii)
	{
		bool const cont = rand() % 2;
		bool const b = rand() % 2;
		FGInstruction instr = FGInstruction::stimulateNeurons(cont, b);

		ASSERT_EQ(cont ? FGInstruction::_stimulateNeuronsContinuous :
				  FGInstruction::_stimulateNeurons,
				  cmd(instr).to_ulong());
		ASSERT_EQ(0, row(instr).to_ulong());
		ASSERT_EQ(b, bank(instr).to_ulong());
		ASSERT_EQ(0, col(instr).to_ulong());
	}
}

} // HICANN
} // HMF
