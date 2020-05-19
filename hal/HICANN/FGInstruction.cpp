#include "hal/HICANN/FGInstruction.h"
#include <ostream>
#include <stdexcept>
#include <bitter/bitter.h>

namespace HMF {
namespace HICANN {

FGInstruction
FGInstruction::writeDown(uint8_t const row, bool const bank)
{
	return FGInstruction(_writeDown, row, 0, bank);
}

FGInstruction
FGInstruction::writeUp(uint8_t const row, bool const bank)
{
	return FGInstruction(_writeUp, row, 0, bank);
}

FGInstruction
FGInstruction::read(uint8_t const row, uint8_t const col, bool const bank)
{
	return FGInstruction(_read, row, col, bank);
}

FGInstruction
FGInstruction::stimulateNeurons(bool const c, bool const bank)
{
	return FGInstruction(c ? _stimulateNeuronsContinuous : _stimulateNeurons,
		0, 0, bank);
}

FGInstruction
FGInstruction::getNextFalse()
{
	static const FGInstruction _i(_getNextFalse, 0, 0, 0);
	return _i;
}

FGInstruction::operator uint32_t () const
{
	return mInstruction.to_ulong();
}

FGInstruction::FGInstruction(
	Instruction const instruction,
	uint8_t const row,
	uint8_t const column,
	bool const bank) :
		mInstruction(0)
{
	if (instruction > 5 || column > 128 || row > 23)
		throw std::runtime_error("invalid FGInstruction");

	std::bitset<3> _instr  = instruction;
	std::bitset<1> _bank   = bank;
	std::bitset<8> _column = column;
	std::bitset<5> _row    = row;

	mInstruction = bit::concat(_instr, _bank, _column, _row);
}

std::ostream& operator<< (std::ostream& os, FGInstruction const& o)
{
	FGInstruction::Instruction const instr = static_cast<FGInstruction::Instruction>(bit::crop<3>(o.mInstruction, 14).to_ulong());
	bool const bank = bit::crop<1>(o.mInstruction, 13).to_ulong();
	uint8_t const column = bit::crop<8>(o.mInstruction, 5).to_ulong();
	uint8_t const row = bit::crop<5>(o.mInstruction).to_ulong();

	os << "FGInstruction:" << std::endl;
	os << "  Instruction = ";
	switch (instr) {
		case FGInstruction::_read:
			os << "read";
			break;
		case FGInstruction::_writeUp:
			os << "writeUp";
			break;
		case FGInstruction::_writeDown:
			os << "writeDown";
			break;
		case FGInstruction::_getNextFalse:
			os << "getNextFalse";
			break;
		case FGInstruction::_stimulateNeurons:
			os << "stimulateNeurons";
			break;
		case FGInstruction::_stimulateNeuronsContinuous:
			os << "stimulateNeuronsContinuous";
			break;
	}
	os << std::endl;
	os << "  bank = " << std::boolalpha << bank << std::endl;
	os << "  column = " << static_cast<int>(column) << std::endl;
	os << "  row = " << static_cast<int>(row) << std::endl;
	return os;
}

} // HICANN
} // HMF
