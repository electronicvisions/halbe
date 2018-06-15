#pragma once

#include "hal/Coordinate/Merger3OnHICANN.h"
#include "hal/HICANN/Merger.h"

namespace HMF {
namespace HICANN {

//AK: Note, that if you want to merge something, all the mergers AFTER the point where merging
//begins have also be set to merge because of the handshake signal (info from JS) ...so there...
//TODO: Alex, could you give an example, please! I don't get exactly what AFTER means. Or, even better, implement a function that checks that.
struct MergerTree
{
public:
	static const size_t num_merger = Coordinate::Merger0OnHICANN::end \
									 + Coordinate::Merger1OnHICANN::end \
									 + Coordinate::Merger2OnHICANN::end \
									 + Coordinate::Merger3OnHICANN::end;

	static PYPP_CONSTEXPR size_t size() { return num_merger; }

	MergerTree();
	PYPP_DEFAULT(MergerTree(MergerTree const&));
	PYPP_DEFAULT(MergerTree& operator=(MergerTree const&));

	// some presets:

	/// straight forwarding from Priority Encoder to Output Register (0->0, 1->1, etc.)
	void set_one_on_one();

	/// always two priority encoders are merged onto one Output Register ( [0,1]->1, [2,3]-3, [4,5]->5, [6,7]->6 )
	void set_two_on_one();

	/// always four priority encoders are merged onto one Output Register ([0,1,2,3]->3, [4,5,6,7]->5)
	void set_four_on_one();

	/// all 8 priority are merged onto one Output Register( range(8) -> 3 )
	void set_eight_on_one();


	Merger&       operator[] (Coordinate::Merger0OnHICANN const& ii);
	Merger const& operator[] (Coordinate::Merger0OnHICANN const& ii) const;

	Merger&       operator[] (Coordinate::Merger1OnHICANN const& ii);
	Merger const& operator[] (Coordinate::Merger1OnHICANN const& ii) const;

	Merger&       operator[] (Coordinate::Merger2OnHICANN const& ii);
	Merger const& operator[] (Coordinate::Merger2OnHICANN const& ii) const;

	Merger&       operator[] (Coordinate::Merger3OnHICANN const& ii);
	Merger const& operator[] (Coordinate::Merger3OnHICANN const& ii) const;

	bool operator ==(MergerTree const& b) const;
	bool operator !=(MergerTree const& b) const;

	friend std::ostream& operator<< (std::ostream& o, const MergerTree& mt);

#ifndef PYPLUSPLUS
	Merger&       getMergerRaw(size_t ii);
	Merger const& getMergerRaw(size_t ii) const;
#endif // PYPLUSPLUS

private:
	std::array<Merger, num_merger> merger;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};


template<typename Archiver>
void MergerTree::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("merger", merger);
}

} // HICANN
} // HMF
