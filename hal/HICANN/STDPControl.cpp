#include "hal/HICANN/STDPControl.h"

namespace HMF {
namespace HICANN {

STDPControl::STDPControl() :
	lut(STDPLUT()),
	eval(STDPEval()),
	analog(STDPAnalog()),
	correlation_info({{{{{{false}},{{false}}}}}}),
	reset_info({{{{{{true}},{{true}}}}}}),
	without_reset(false),
	read_causal(true),
	read_acausal(true),
	continuous_autoupdate(true),
	m_first_row(0),
	m_last_row(0)
{}

const size_t
	STDPControl::NUMBER_SLICES,
	STDPControl::SYNAPSES_PER_SLICE,
	STDPControl::SYNAPSES_PER_ROW,
	STDPControl::NUMBER_ROWS;

	//set first and last rows with the help of syndriver coordinates
	void STDPControl::set_first_row(Coordinate::SynapseDriverOnHICANN const& s, bool const line)
	{
		m_first_row = (s.line() < 112) ?
			223-(s.line()*2)     - (uint8_t)line :
			    (s.line()-112)*2 + (uint8_t)line;
	}

	void STDPControl::set_last_row(Coordinate::SynapseDriverOnHICANN const& s, bool const line)
	{
		m_last_row = (s.line() < 112) ?
			223-(s.line()*2) -     (uint8_t)line :
			    (s.line()-112)*2 + (uint8_t)line;
	}

	//set first and last rows with uints
	void STDPControl::set_first_row(uint8_t const row)
	{
		m_first_row = row;
	}

	void STDPControl::set_last_row(uint8_t const row)
	{
		m_last_row = row;
	}

	bool STDPControl::operator ==(STDPControl const& b) const
	{
		return b.lut == lut
			&& b.eval == eval
			&& b.analog == analog
			&& b.timing == timing
			&& b.m_first_row == m_first_row
			&& b.m_last_row == m_last_row
			&& b.reset_info == reset_info
			&& b.correlation_info == correlation_info
			&& b.without_reset == without_reset
			&& b.read_causal == read_causal
			&& b.read_acausal == read_acausal
			&& b.continuous_autoupdate == continuous_autoupdate;
	}


namespace {
void print_bool_array_helper(std::ostream& os, std::array<bool, STDPControl::SYNAPSES_PER_ROW> a)
{
	auto prev = *std::begin(a);
	auto count = 0;
	for (auto b : a) {
		if (b == prev) {
			count++;
		} else {
			os << count << "x ";
			os << std::boolalpha << prev << ", ";
			prev = b;
			count = 1;
		}
	}
	os << count << "x ";
	os << std::boolalpha << prev;
}

void print_row_array_helper(std::ostream& os, std::array<STDPControl::corr_row, STDPControl::NUMBER_ROWS> cra)
{
	os << std::endl;
	auto prev = *std::begin(cra);
	auto count = 0;
	for (auto row : cra) {
		if (row == prev) {
			count++;
		} else {
			os << count << "x ";
			os << "causal: ";
			print_bool_array_helper(os, prev[0]);
			os << ", acausal: ";
			print_bool_array_helper(os, prev[1]);
			os << std::endl;
			prev = row;
			count = 1;
		}
	}
	os << count << "x ";
	os << "causal: ";
	print_bool_array_helper(os, prev[0]);
	os << ", acausal: ";
	print_bool_array_helper(os, prev[1]);
	os << std::endl;
}
}

std::ostream& operator<< (std::ostream& os, STDPControl const& o)
{
	os << "STDPControl:" << std::endl;
	os << "  lut = " << o.lut << std::endl;
	os << "  eval = " << o.eval << std::endl;
	os << "  analog = " << o.analog << std::endl;
	os << "  timing = " << o.timing << std::endl;
	os << "  m_first_row = " << static_cast<int>(o.m_first_row) << std::endl;
	os << "  m_last_row = " << static_cast<int>(o.m_last_row) << std::endl;
	os << "  reset_info = ";
	print_row_array_helper(os, o.reset_info);
	os << "  correlation_info = ";
	print_row_array_helper(os, o.correlation_info);
	os << "  without_reset = " << o.without_reset << std::endl;
	os << "  read_causal = " << o.read_causal << std::endl;
	os << "  read_acausal = " << o.read_acausal << std::endl;
	os << "  continuous_autoupdate = " << o.continuous_autoupdate << std::endl;
	return os;
}

} // end namespace HMF
} // end namespace HICANN

