#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>

#include "hal/Coordinate/geometry.h"
#include "hal/Coordinate/typed_array.h"
#include "hal/HICANN/FGBlock.h"


namespace HMF
{
namespace HICANN
{

/// Struct that holds a single error result of the floating gate controller.
///  See section 4.4.2 and table 4.12 in the HBP Specification.
struct FGErrorResult
{
	/// HICANN OCP bus returns 32bit words
	typedef uint32_t raw_data_t;

	/// Default constructor creating a no-error-state instance
	FGErrorResult();

	///
	/// @param result, as returned by HMF::HICANN::fg_read_answer
	///
	FGErrorResult(raw_data_t result);

	///
	/// get raw data, as returned by HMF::HICANN::fg_read_answer
	///
	raw_data_t get_slave_answer_data() const;

	///
	/// @return check if data seems valid (i.e. contains a legal value)
	///
	bool check() const;

	///
	/// @return the cell address of incorrectly programmed voltage
	///
	HMF::Coordinate::X get_cell() const;

	///
	/// @return controler busy flag
	///
	bool get_busy_flag() const;

	///
	/// @return error flag
	///
	bool get_error_flag() const;

	friend std::ostream& operator<< (std::ostream& os, FGErrorResult const& fger);

private:
	raw_data_t m_slave_answer_data;

#ifndef PYPLUSPLUS
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
#endif // PYPLUSPLUS
};


/// Struct that holds up to a row of error results of the floating gate controller.
///  See FGErrorResult for a description of a single error result.
struct FGErrorResultRow {
	FGErrorResultRow();

	FGErrorResult const& operator[] (HMF::Coordinate::X const& fg_col_index) const;
	FGErrorResult&       operator[] (HMF::Coordinate::X const& fg_col_index);

	friend std::ostream& operator<< (std::ostream& os, FGErrorResultRow const& fgerr);

private:
	std::array<HMF::HICANN::FGErrorResult, FGBlock::fg_columns-1> m_row_results;

#ifndef PYPLUSPLUS
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
#endif // PYPLUSPLUS
};


/// Struct that holds four rows of error results of the four floating gate controllers.
/// See FGErrorResultRow for a description of a single row of errors.
struct FGErrorResultQuadRow {
	FGErrorResultQuadRow();

	FGErrorResultRow const& operator[] (Coordinate::FGBlockOnHICANN const& blk) const;
	FGErrorResultRow&       operator[] (Coordinate::FGBlockOnHICANN const& blk);

	friend std::ostream& operator<< (std::ostream& os, FGErrorResultQuadRow const& fgeqr);

private:
	Coordinate::typed_array<FGErrorResultRow, Coordinate::FGBlockOnHICANN> m_quad_row_results;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};



} // HICANN
} // HMF


#ifndef PYPLUSPLUS
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_KEY(::HMF::HICANN::FGErrorResult)
BOOST_CLASS_EXPORT_KEY(::HMF::HICANN::FGErrorResultRow)
BOOST_CLASS_EXPORT_KEY(::HMF::HICANN::FGErrorResultQuadRow)
#endif
