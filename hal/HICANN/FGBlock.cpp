#include "hal/HICANN/FGBlock.h"
#include "halco/common/iter_all.h"
#include "halco/common/geometry.h"
#include "halco/hicann/v2/fg.h"
#include "halco/hicann/v2/neuron.h"

#include "pythonic/enumerate.h"

#include <bitter/bitter.h>
#include <string>

using namespace halco::hicann::v2;
using namespace halco::common;

namespace HMF {
namespace HICANN {

int const not_connected = -1;

size_t const FGBlock::fg_blocks;
size_t const FGBlock::fg_lines;
size_t const FGBlock::fg_columns;

FGBlock::shared_lut_t const
FGBlock::shared_lut_left = {{
	0, 1, 2, 3, not_connected,
	4, 5, 6, 7, 8, 9, 10, 11,
	12, not_connected, 13, 14,
	15, 16, 17, 19, 21, 23
}};

FGBlock::shared_lut_t const
FGBlock::shared_lut_right = {{
	0, 1, 2, not_connected, 3,
	4, 5, 6, 7, 8, 9, 10, 11,
	not_connected, 12, 13, 14,
	15, 16, 17, 19, 21, 23
}};

FGBlock::neuron_lut_t const
FGBlock::neuron_lut_left = {{
	6, 18, 16, 1, 3, 17, 7, 11, 9, 21, 19,
	13, 15, 23, 5, 20, 8, 10, 14, 22, 12
// #ifdef HICANNv4
	, 4, 2
// #endif
}};

FGBlock::neuron_lut_t const
FGBlock::neuron_lut_right = {{
	16, 2, 0, 15, 1, 3, 19, 21, 11, 7, 5,
	9, 23, 13, 17, 4, 10, 8, 12, 6, 14
// #ifdef HICANNv4
	, 18, 20
// #endif
}};

std::array<std::pair<shared_parameter, FGBlock::value_type>,
	shared_parameter::__last_shared> const
FGBlock::shared_default = {{
	{ I_breset,      1023 },
	{ I_bstim,       1023 },
	{ int_op_bias,   1023 },
	{ V_bout,        1023 },
	{ V_bexp,        1023 },
	{ V_br,             0 },
	{ V_bstdf,          0 },
	{ V_ccas,         800 },
	{ V_clrc,           0 },
	{ V_clra,           0 },
	{ V_dep,            0 },
	{ V_dllres,       200 }, /* pll frequency dependent, smaller values for slower clocks. If
								the this value is too high for slow clocks, repeats could lock
								to double events in the very extreme case. If the value is too
								slow for fast clocks, than events can't be locked at all */
	{ V_dtc,            0 },
	{ V_fac,            0 },
	{ V_gmax0,       1000 },
	{ V_gmax1,       1000 },
	{ V_gmax2,       1000 },
	{ V_gmax3,       1000 },
	{ V_m,              0 },
	{ V_reset,        300 },
	{ V_stdf,           0 },
	{ V_thigh,          0 },
	{ V_tlow,           0 }
}};

std::array<std::pair<neuron_parameter, FGBlock::value_type>,
	neuron_parameter::__last_neuron> const
FGBlock::neuron_default = {{
	{ E_l,            300 },
	{ E_syni,         100 },
	{ E_synx,         570 },
	{ I_bexp,        1023 },
	{ I_convi,       1023 },
	{ I_convx,       1023 },
	{ I_fire,         511 },
	{ I_gl,           200 },
	{ I_gladapt,      100 },
	{ I_intbbi,       511 },
	{ I_intbbx,       511 },
	{ I_pl,           511 },
	{ I_radapt,       511 },
	{ I_rexp,         511 },
	{ I_spikeamp,    1023 },
	{ V_exp,          400 },
	{ V_syni,         511 },
	{ V_syntci,       800 },
	{ V_syntcx,       800 },
	{ V_synx,         511 },
	{ V_t,            500 }
// #ifdef HICANNv4
	, { V_convoffi,     1023 }
	, { V_convoffx,     1023 }
// #endif
}};

neuron_parameter getNeuronParameter(halco::hicann::v2::FGBlockOnHICANN const& b,
		                            halco::hicann::v2::FGRowOnFGBlock const & r)
{
	auto const & lut = FGBlock::getNeuronLut(b);
	auto it = std::find(lut.begin(), lut.end(), r.value());
	if (it == lut.end())
		throw std::out_of_range("Not connected");
	else
		return static_cast<neuron_parameter>(std::distance(lut.begin(), it));
}

shared_parameter getSharedParameter(halco::hicann::v2::FGBlockOnHICANN const& b,
		                            halco::hicann::v2::FGRowOnFGBlock const & r)
{
	auto const & lut = FGBlock::getSharedLut(b);
	auto it = std::find(lut.begin(), lut.end(), r.value());
	if (it == lut.end())
		throw std::out_of_range("Not connected");
	else
		return static_cast<shared_parameter>(std::distance(lut.begin(), it));
}

bool isCurrentParameter(neuron_parameter p)
{
	switch(p) {
		case E_l: return false;
		case E_syni: return false;
		case E_synx: return false;
		case I_bexp: return true;
		case I_convi: return true;
		case I_convx: return true;
		case I_fire: return true;
		case I_gl: return true;
		case I_gladapt: return true;
		case I_intbbi: return true;
		case I_intbbx: return true;
		case I_pl: return true;
		case I_radapt: return true;
		case I_rexp: return true;
		case I_spikeamp: return true;
		case V_exp: return false;
		case V_syni: return false;
		case V_syntci: return false;
		case V_syntcx: return false;
		case V_synx: return false;
		case V_t: return false;
		case V_convoffi: return false;
		case V_convoffx: return false;
		default: throw std::out_of_range("Not a parameter.");
	}
}

bool isCurrentParameter(shared_parameter p)
{
	switch(p) {
		// TODO @DS check with hardie
		case I_breset: return true;
		case I_bstim: return true;
		case V_bexp: return true;
		case V_bout: return true;
		case V_br: return false;
		case V_bstdf: return false;
		case V_ccas: return false;
		case V_clra: return false;
		case V_clrc: return false;
		case V_dep: return false;
		case V_dllres: return false;
		case V_dtc: return false;
		case V_fac: return false;
		case V_gmax0: return true;
		case V_gmax1: return true;
		case V_gmax2: return true;
		case V_gmax3: return true;
		case V_m: return false;
		case V_reset: return false;
		case V_stdf: return false;
		case V_thigh: return false;
		case V_tlow: return false;
		case int_op_bias: return true;
		default: throw std::out_of_range("Not a parameter.");
	}
}

std::string to_string(neuron_parameter p)
{
	switch(p) {
		case E_l: return "E_l";
		case E_syni: return "E_syni";
		case E_synx: return "E_synx";
		case I_bexp: return "I_bexp";
		case I_convi: return "I_convi";
		case I_convx: return "I_convx";
		case I_fire: return "I_fire";
		case I_gl: return "I_gl";
		case I_gladapt: return "I_gladapt";
		case I_intbbi: return "I_intbbi";
		case I_intbbx: return "I_intbbx";
		case I_pl: return "I_pl";
		case I_radapt: return "I_radapt";
		case I_rexp: return "I_rexp";
		case I_spikeamp: return "I_spikeamp";
		case V_exp: return "V_exp";
		case V_syni: return "V_syni";
		case V_syntci: return "V_syntci";
		case V_syntcx: return "V_syntcx";
		case V_synx: return "V_synx";
		case V_t: return "V_t";
		case V_convoffi: return "V_convoffi";
		case V_convoffx: return "V_convoffx";
		default: {
			throw std::out_of_range(
			    "to_string: not a neuron parameter " + std::to_string(static_cast<int>(p)));
		}
	}
}

std::string to_string(shared_parameter p)
{
	switch(p) {
		case I_breset: return "I_breset";
		case I_bstim: return "I_bstim";
		case V_bexp: return "V_bexp";
		case V_bout: return "V_bout";
		case V_br: return "V_br";
		case V_bstdf: return "V_bstdf";
		case V_ccas: return "V_ccas";
		case V_clra: return "V_clra";
		case V_clrc: return "V_clrc";
		case V_dep: return "V_dep";
		case V_dllres: return "V_dllres";
		case V_dtc: return "V_dtc";
		case V_fac: return "V_fac";
		case V_gmax0: return "V_gmax0";
		case V_gmax1: return "V_gmax1";
		case V_gmax2: return "V_gmax2";
		case V_gmax3: return "V_gmax3";
		case V_m: return "V_m";
		case V_reset: return "V_reset";
		case V_stdf: return "V_stdf";
		case V_thigh: return "V_thigh";
		case V_tlow: return "V_tlow";
		case int_op_bias: return "int_op_bias";
		default: {
			throw std::out_of_range(
			    "to_string: not a shared parameter " + std::to_string(static_cast<int>(p)));
		}
	}
}

bool isVoltageParameter(neuron_parameter p)
{
	return !isCurrentParameter(p);
}

bool isVoltageParameter(shared_parameter p)
{
	return !isCurrentParameter(p);
}

bool isFGParameter(shared_parameter p)
{
	switch (p) {
		case int_op_bias:
			return true;
		default:
			return false;
	}
}

bool isL1Parameter(shared_parameter p)
{
	switch (p) {
		case V_dllres:
			return true;
		case V_ccas: // also cbias
			return true;
		default:
			return false;
	}
}

bool isPotentialL1Row(halco::hicann::v2::FGRowOnFGBlock const& row) {
	for (auto block : halco::common::iter_all<halco::hicann::v2::FGBlockOnHICANN>()) {
		try {
			if (isL1Parameter(getSharedParameter(block, row))) {
				return true;
			}
		} catch (std::out_of_range const&) {
		}
	}
	return false;
}

bool isPotentialFGRow(halco::hicann::v2::FGRowOnFGBlock const& row) {
	for (auto block : halco::common::iter_all<halco::hicann::v2::FGBlockOnHICANN>()) {
		try {
			if (isFGParameter(getSharedParameter(block, row))) {
				return true;
			}
		} catch (std::out_of_range const&) {
		}
	}
	return false;
}

halco::hicann::v2::FGRowOnFGBlock
getNeuronRow(halco::hicann::v2::FGBlockOnHICANN const& b, neuron_parameter p)
{
	return halco::hicann::v2::FGRowOnFGBlock(FGBlock::getNeuronLut(b).at(p));
}

halco::hicann::v2::FGRowOnFGBlock
getSharedRow(halco::hicann::v2::FGBlockOnHICANN const& b, shared_parameter p)
{
	return halco::hicann::v2::FGRowOnFGBlock(FGBlock::getSharedLut(b).at(p));
}

FGBlock::FGBlock(halco::hicann::v2::FGBlockOnHICANN const& b) :
	mShared(), mNeuron(), mCoordinate(b)
{
	setDefault(b);
}

FGBlock::value_type
FGBlock::getShared(
	FGBlockOnHICANN const& b,
	shared_parameter param) const
{
	return mShared.at(getSharedLut(b).at(param));
}

void FGBlock::setShared(
	FGBlockOnHICANN const& b,
	shared_parameter param,
	value_type const& val)
{
	mShared.at(getSharedLut(b).at(param)) = val;
}

FGBlock::value_type FGBlock::getSharedRaw(size_t idx) const
{
	return getRaw(FGCellOnFGBlock(X(0), Y(idx)));
}

void FGBlock::setSharedRaw(size_t idx, value_type const& val)
{
	setRaw(FGCellOnFGBlock(X(0), Y(idx)), val);
}

FGBlock::value_type FGBlock::getRaw(size_t row, size_t column) const
{
	return getRaw(FGCellOnFGBlock(X(column), Y(row)));
}

void FGBlock::setRaw(size_t row, size_t column, value_type val)
{
	setRaw(FGCellOnFGBlock(X(column), Y(row)), val);
}

FGBlock::value_type FGBlock::getRaw(halco::hicann::v2::FGCellOnFGBlock cell) const
{
	if (cell.x() == X(0))
		return mShared.at(cell.y());
	else
		return mNeuron.at(cell.x() - 1).at(cell.y());
}

void FGBlock::setRaw(halco::hicann::v2::FGCellOnFGBlock cell, value_type val)
{
	if (cell.x() == X(0))
		mShared.at(cell.y()) = val;
	else
		mNeuron.at(cell.x() - 1).at(cell.y()) = val;
}

FGBlock::value_type FGBlock::getNeuron(
	FGBlockOnHICANN const& b,
	NeuronOnFGBlock const& nrn,
	neuron_parameter np) const
{
	return getNeuronRaw(is_left(b) ? nrn : 127-nrn, getNeuronLut(b).at(np));
}

void FGBlock::setNeuron(
	FGBlockOnHICANN const& b,
	NeuronOnFGBlock const& nrn,
	neuron_parameter np,
	value_type const& val)
{
	setNeuronRaw(is_left(b) ? nrn : 127-nrn, getNeuronLut(b).at(np), val);
}

FGBlock::value_type FGBlock::getNeuronRaw(
	size_t const col,
	size_t const row) const
{
	return getRaw(FGCellOnFGBlock(X(col + 1), Y(row)));
}
void FGBlock::setNeuronRaw(
	size_t const col,
	size_t const row,
	value_type const& val)
{
	setRaw(FGCellOnFGBlock(X(col + 1), Y(row)), val);
}

void FGBlock::setDefault(FGBlockOnHICANN const& b)
{
	for (auto const& d : neuron_default)
	{
		for (size_t nrn=0; nrn<128; ++nrn)
			setNeuron(b, NeuronOnFGBlock(nrn), d.first, d.second);
	}

	for (auto const& d : shared_default)
	{
		if (getSharedLut(b).at(d.first) != not_connected)
			setShared(b, d.first, d.second);
	}
}


int FGBlock::getSharedHardwareIdx(halco::hicann::v2::FGBlockOnHICANN const& b, shared_parameter const& i) {
	return getSharedLut(b).at(i);
}


int FGBlock::getNeuronHardwareIdx(halco::hicann::v2::FGBlockOnHICANN const& b, neuron_parameter const& i) {
	return getNeuronLut(b).at(i);
}


bool FGBlock::operator== (FGBlock const& rhs) const
{
	return mShared == rhs.mShared
		&& mNeuron == rhs.mNeuron;
}

bool FGBlock::operator!= (FGBlock const& rhs) const
{
	return !(*this == rhs);
}

bool FGBlock::is_left(FGBlockOnHICANN const& b)
{
	return b.x() == left;
}

FGBlock::shared_lut_t const&
FGBlock::getSharedLut(FGBlockOnHICANN const& b)
{
	return is_left(b) ? shared_lut_left : shared_lut_right;
}

FGBlock::neuron_lut_t const&
FGBlock::getNeuronLut(FGBlockOnHICANN const& b)
{
	return is_left(b) ? neuron_lut_left : neuron_lut_right;
}

std::array<std::bitset<20>, 65>
FGBlock::set_formatter(
	FGBlockOnHICANN const& /* b */,
	rant::integral_range<size_t, 23> const& row) const
{
	std::array<std::bitset<20>, 65> r;

	// TODO: make sure its address 0 on left as well as right side
	// set shared parameter
	r[0] = getSharedRaw(row);

	// set neuron parameter
	for (size_t nrn=0; nrn<128; ++nrn)
		r[(nrn+1)/2] |= (getNeuronRaw(nrn, row) << (nrn%2 ? 0 : 10));

	return r;
}

FGRow FGBlock::getFGRow(FGRowOnFGBlock row) const
{
	FGRow fg_row;
	fg_row.setShared(getSharedRaw(row));
	// set neuron parameter
	for (size_t nrn=0; nrn<128; ++nrn)
		fg_row.setNeuron(NeuronOnFGBlock(nrn), getNeuronRaw(nrn, row));
	return fg_row;
}

namespace {
void printSharedHelper(std::ostream& os, FGBlock::fg_t array, FGBlock::shared_lut_t lut)
{
	for (auto const& val : lut)
	{
		if (val == not_connected) {
			continue;
		}

		os.width(4);
		os << array[val] << " ";
	}
}

void printNeuronHelper(std::ostream& os, FGBlock::fg_t array, FGBlock::neuron_lut_t lut)
{
	for (auto const& val : lut)
	{
		if (val == not_connected) {
			continue;
		}
		os.width(4);
		os << array[val] << " ";
	}
}

void printCoordinateHelper(std::ostream& os, int const& first, int const& last)
{
	os << "    NeuronOnFGBlock(" << first;
	if (first != last) {
		os << "-" << last;
	}
	os << "):\n      ";
}

} // end namespace

std::ostream& operator<< (std::ostream& os, FGBlock const& fgb)
{
	os << fgb.mCoordinate << ":\n";
	os << "\n  Shared FG:\n    ";
	auto shared_lut = FGBlock::getSharedLut(fgb.mCoordinate);
	printSharedHelper(os, fgb.mShared, shared_lut);
	os << "\n\n";
	os << "  Neuron FG:\n";

	// print repeating neuron properties only once
	auto first = 0; // first neuron with same properties
	auto current = 0;
	auto prev = fgb.mNeuron[first];
	auto neuron_lut = FGBlock::getNeuronLut(fgb.mCoordinate);
	for (auto it : pythonic::enumerate(fgb.mNeuron))
	{
		auto const& array = it.second; // fg_lines array
		current = it.first; // neuron id
		if (prev != array) {
			printCoordinateHelper(os, first, current-1);
			printNeuronHelper(os, prev, neuron_lut);
			os << "\n";
			prev = array;
			first = current;
		}
	}
	printCoordinateHelper(os, first, current);
	printNeuronHelper(os, prev, neuron_lut);
	os << "\n";
	return os;
}

} // HICANN
} // HMF
