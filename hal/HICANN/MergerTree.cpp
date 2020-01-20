#include "hal/HICANN/MergerTree.h"

using namespace halco::hicann::v2;

namespace HMF {
namespace HICANN {

const size_t MergerTree::num_merger;

MergerTree::MergerTree() :
	merger({{
		Merger::MERGE, Merger::MERGE, Merger::MERGE, Merger::MERGE,
		Merger::MERGE, Merger::MERGE, Merger::MERGE, Merger::MERGE,
		Merger::RIGHT_ONLY, Merger::RIGHT_ONLY, Merger::RIGHT_ONLY, Merger::LEFT_ONLY,
		Merger::RIGHT_ONLY, Merger::LEFT_ONLY, Merger::LEFT_ONLY}})
{}


void MergerTree::set_one_on_one()
{
	enum { BG0, BG1, BG2, BG3, BG4, BG5, BG6, BG7,
		L0_0, L0_1, L0_2, L0_3, L1_0, L1_1, L2_0 };

	merger[L0_0] = Merger(Merger::RIGHT_ONLY);
	merger[L0_1] = Merger(Merger::RIGHT_ONLY);
	merger[L0_2] = Merger(Merger::RIGHT_ONLY);
	merger[L0_3] = Merger(Merger::LEFT_ONLY);
	merger[L1_0] = Merger(Merger::RIGHT_ONLY);
	merger[L1_1] = Merger(Merger::LEFT_ONLY);
	merger[L2_0] = Merger(Merger::LEFT_ONLY);
}
void MergerTree::set_two_on_one()
{
	enum { BG0, BG1, BG2, BG3, BG4, BG5, BG6, BG7,
		L0_0, L0_1, L0_2, L0_3, L1_0, L1_1, L2_0 };

	merger[L0_0] = Merger(Merger::MERGE);
	merger[L0_1] = Merger(Merger::MERGE);
	merger[L0_2] = Merger(Merger::MERGE);
	merger[L0_3] = Merger(Merger::MERGE);
	merger[L1_0] = Merger(Merger::RIGHT_ONLY);
	merger[L1_1] = Merger(Merger::LEFT_ONLY);
	merger[L2_0] = Merger(Merger::LEFT_ONLY);
}

void MergerTree::set_four_on_one()
{
	enum { BG0, BG1, BG2, BG3, BG4, BG5, BG6, BG7,
		L0_0, L0_1, L0_2, L0_3, L1_0, L1_1, L2_0 };

	merger[L0_0] = Merger(Merger::MERGE);
	merger[L0_1] = Merger(Merger::MERGE);
	merger[L0_2] = Merger(Merger::MERGE);
	merger[L0_3] = Merger(Merger::MERGE);
	merger[L1_0] = Merger(Merger::MERGE);
	merger[L1_1] = Merger(Merger::MERGE);
	merger[L2_0] = Merger(Merger::LEFT_ONLY);
}

void MergerTree::set_eight_on_one()
{
	enum { BG0, BG1, BG2, BG3, BG4, BG5, BG6, BG7,
		L0_0, L0_1, L0_2, L0_3, L1_0, L1_1, L2_0 };

	merger[L0_0] = Merger(Merger::MERGE);
	merger[L0_1] = Merger(Merger::MERGE);
	merger[L0_2] = Merger(Merger::MERGE);
	merger[L0_3] = Merger(Merger::MERGE);
	merger[L1_0] = Merger(Merger::MERGE);
	merger[L1_1] = Merger(Merger::MERGE);
	merger[L2_0] = Merger(Merger::MERGE);
}

Merger& MergerTree::operator[] (Merger0OnHICANN const& ii)
{
	return merger[ii];
}

Merger const& MergerTree::operator[] (Merger0OnHICANN const& ii) const
{
	return merger[ii];
}

Merger& MergerTree::operator[] (Merger1OnHICANN const& ii)
{
	return merger[Merger0OnHICANN::end + ii];
}

Merger const& MergerTree::operator[] (Merger1OnHICANN const& ii) const
{
	return merger[Merger0OnHICANN::end + ii];
}

Merger& MergerTree::operator[] (Merger2OnHICANN const& ii)
{
	return merger[Merger0OnHICANN::end + Merger1OnHICANN::end + ii];
}

Merger const& MergerTree::operator[] (Merger2OnHICANN const& ii) const
{
	return merger[Merger0OnHICANN::end + Merger1OnHICANN::end + ii];
}

Merger& MergerTree::operator[] (Merger3OnHICANN const& ii)
{
	MergerTree const& This = *this;
	return const_cast<Merger&>(This[ii]);
}

Merger const& MergerTree::operator[] (Merger3OnHICANN const& ii) const
{
	// ii is always 0 and the offset is const. Therefore, this is only a very
	// complex way and inefficient way to write 14.
	// Still, let's exercise sake of consistency.
	static size_t const offset =
		Merger0OnHICANN::end +
		Merger1OnHICANN::end +
		Merger2OnHICANN::end;

	return merger[offset + ii];
}

bool MergerTree::operator ==(MergerTree const& b) const
{
	return merger == b.merger;
}

bool MergerTree::operator !=(MergerTree const& b) const
{
	return !(*this == b);
}

std::ostream& operator<< (std::ostream& o, const MergerTree& mt)
{
	static std::array<std::string, MergerTree::size()> const names = {{
		{"BG0"}, {"BG1"}, {"BG2"}, {"BG3"}, {"BG4"}, {"BG5"}, {"BG6"}, {"BG7"},
		{"L0_0"}, {"L0_1"}, {"L0_2"}, {"L0_3"}, {"L1_0"}, {"L1_1"}, {"L2_0"}
	}};

	o << "MergerTree:" << std::endl;
	for (size_t ii=0; ii<MergerTree::size(); ++ii)
		o << "\t" << names[ii] << ": " << mt.merger[ii] << std::endl;
	return o;
}

Merger& MergerTree::getMergerRaw(size_t const ii)
{
	return merger.at(ii);
}

Merger const& MergerTree::getMergerRaw(size_t const ii) const
{
	return merger.at(ii);
}

} // HICANN
} // HMF
