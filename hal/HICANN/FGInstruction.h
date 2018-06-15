#pragma once

#include <bitset>
#include <cstdlib>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>

#include "hal/test.h"
#include "pywrap/compat/macros.hpp"

namespace HMF {
namespace HICANN {

class FGInstruction
{
public:
	PYPP_DEFAULT(FGInstruction());
	PYPP_DEFAULT(FGInstruction(FGInstruction const&));
	PYPP_DEFAULT(FGInstruction& operator=(FGInstruction const&));

	static FGInstruction writeDown(uint8_t row, bool bank = 0);
	static FGInstruction writeUp(uint8_t row, bool bank = 0);
	static FGInstruction read(uint8_t row, uint8_t col, bool bank = 0);
	static FGInstruction stimulateNeurons(bool continuous, bool bank = 0);
	static FGInstruction getNextFalse();

	operator uint32_t () const;

private:
	// taken from: global_src/systemc/hardware_base.h
	enum Instruction {
		_read                       = 0,
		_writeUp                    = 1,
		_writeDown                  = 2,
		_getNextFalse               = 3,
		_stimulateNeurons           = 4,
		_stimulateNeuronsContinuous = 5
	};

	FGInstruction(Instruction instruction,
				  uint8_t row,
				  uint8_t column,
				  bool bank = 0);

	std::bitset<17> mInstruction;

	FRIEND_TEST(FGInstruction, Default);
	FRIEND_TEST(FGInstruction, writeDown);
	FRIEND_TEST(FGInstruction, writeUp);
	FRIEND_TEST(FGInstruction, read);
	FRIEND_TEST(FGInstruction, stimulateNeurons);
	FRIEND_TEST(FGInstruction, getNextFalse);

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver & ar, unsigned int const);

	friend std::ostream& operator<< (std::ostream& os, FGInstruction const& o);
};


template<typename Archiver>
void FGInstruction::serialize(Archiver & ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("instruction", mInstruction);
}

} // HICANN
} // HMF
