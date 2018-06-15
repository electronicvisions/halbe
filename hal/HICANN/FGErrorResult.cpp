#include <bitter/bitter.h>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>

#include "hal/Coordinate/iter_all.h"
#include "FGErrorResult.h"


namespace HMF
{
namespace HICANN
{

FGErrorResult::FGErrorResult() : m_slave_answer_data(0)
{
}

FGErrorResult::FGErrorResult(raw_data_t result) : m_slave_answer_data(result)
{
}

auto FGErrorResult::get_slave_answer_data() const -> raw_data_t
{
	return m_slave_answer_data;
}

namespace {
	constexpr static const size_t slave_answer_busy_flag = 8;
	constexpr static const size_t slave_answer_error_flag = 9;
} // anonymous

bool FGErrorResult::check() const
{
	// valid values are < 129 (and two more flag bits above these 7 bits may be set)
	raw_data_t tmp = bit::set(m_slave_answer_data, slave_answer_busy_flag, slave_answer_error_flag, 0, 0);
	return tmp < FGBlock::fg_columns;
}

HMF::Coordinate::X FGErrorResult::get_cell() const
{
	// See Table 4.12: The lowest 8 bit contain the column number
	// of an incorrectly programmed cell
	return HMF::Coordinate::X(
		HMF::Coordinate::Enum(
			bit::crop<8>(bit::convert(m_slave_answer_data)).to_ulong()));
}

bool FGErrorResult::get_busy_flag() const
{
	// See Table 4.12: Bit 8 contains the busy flag
	return bit::test(m_slave_answer_data, slave_answer_busy_flag);
}

bool FGErrorResult::get_error_flag() const
{
	// See Table 4.12: Bit 9 contains the error flag
	return bit::test(m_slave_answer_data, slave_answer_error_flag);
}

std::ostream& operator<<(std::ostream& os, FGErrorResult const& fger) {
	os << "busy flag: " << fger.get_busy_flag() << "\n"
	   << "error flag: " << fger.get_error_flag() << "\n"
	   << "check: " << fger.check() << "\n"
	   << "column of incorrectly programmed cell: " << fger.get_cell();

	return os;
}

template<typename Archiver>
void FGErrorResult::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("m_slave_answer_data", m_slave_answer_data);
}


FGErrorResultRow::FGErrorResultRow()
{
	// default constructs the data member variable
}

FGErrorResult const& FGErrorResultRow::operator[] (HMF::Coordinate::X const& fg_col_index) const {
	if (fg_col_index.value() >= FGBlock::fg_columns)
		throw std::length_error("FGErrorResultRow[]: invalid column");
	return m_row_results[fg_col_index.value()];
}

FGErrorResult& FGErrorResultRow::operator[] (HMF::Coordinate::X const& fg_col_index) {
	if (fg_col_index.value() >= FGBlock::fg_columns)
		throw std::length_error("FGErrorResultRow[]: invalid column");
	return m_row_results[fg_col_index.value()];
}

std::ostream& operator<< (std::ostream& os, FGErrorResultRow const& fgerr) {

	for(size_t column = 0; column != FGBlock::fg_columns; ++column) {
		const FGErrorResult& fger = fgerr[HMF::Coordinate::X(column)];
		os << (fger.get_error_flag() ? "X" : "-");
	}

	return os;
}

template<typename Archiver>
void FGErrorResultRow::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("m_row_results", m_row_results);
}


FGErrorResultQuadRow::FGErrorResultQuadRow()
{
	// default constructs the data member variable
}

FGErrorResultRow const& FGErrorResultQuadRow::operator[] (Coordinate::FGBlockOnHICANN const& blk) const
{
	return m_quad_row_results[blk];
}

FGErrorResultRow& FGErrorResultQuadRow::operator[] (Coordinate::FGBlockOnHICANN const& blk)
{
	return m_quad_row_results[blk];
}

std::ostream& operator<< (std::ostream& os, FGErrorResultQuadRow const& fgeqr) {

	for (auto block : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>()) {
		os << block << ":\t";
		os << fgeqr[block] << '\n';
	}

	return os;
}

template<typename Archiver>
void FGErrorResultQuadRow::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("m_quad_row_results", m_quad_row_results);
}

} // HICANN
} // HMF

BOOST_CLASS_EXPORT_IMPLEMENT(::HMF::HICANN::FGErrorResult)
BOOST_CLASS_EXPORT_IMPLEMENT(::HMF::HICANN::FGErrorResultRow)
BOOST_CLASS_EXPORT_IMPLEMENT(::HMF::HICANN::FGErrorResultQuadRow)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::HMF::HICANN::FGErrorResult)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::HMF::HICANN::FGErrorResultRow)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::HMF::HICANN::FGErrorResultQuadRow)
