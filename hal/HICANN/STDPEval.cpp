#include "hal/HICANN/STDPEval.h"

#include <stdexcept>

namespace HMF {
namespace HICANN {

const size_t //enum to identify single factors
	STDPEval::ca,
	STDPEval::ac,
	STDPEval::aa,
	STDPEval::cc;

std::bitset<8> STDPEval::extract_bits() const
{
	std::bitset<8> data;

	// for (size_t i = 0; i < 4; i++){
	// 	data[2*i]     = causal[i];
	// 	data[2*i + 1] = acausal[i];
	// }
	static size_t const indices[] = { aa, ac, ca, cc };

	for(int i=0; i<4; i++) {
		data[i*2+1] =  causal[indices[i]];
		data[i*2  ] = acausal[indices[i]];
	}

	return data;
}

void STDPEval::import_bits(std::bitset<8>){
	throw std::runtime_error("STDPEval::import_bits: Not implemented :P");
	// TODO
	//for (size_t i = 0; i < 4; i++){
	//	causal[i]  = data[2*i];
	//	acausal[i] = data[2*i + 1];
	//}
}

std::ostream& operator<< (std::ostream& os, STDPEval const& o)
{
	os << "STDPEval(";
	os << "causal = " << o.causal << ", ";
	os << "acausal = " << o.acausal << ")";
	return os;
}

} // end namespace HMF
} // end namespace HICANN
