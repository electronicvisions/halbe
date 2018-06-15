#pragma once

#include "halco/common/geometry.h"


namespace HMF {
namespace Coordinate {

using halco::common::X;
using halco::common::Y;
using halco::common::Enum;
using halco::common::EnumRanged;

namespace detail {

using halco::common::detail::RantWrapper;

} // namespace detail
} // namespace Coordinate
} // namespace HMF


namespace geometry {

// Legacy coordinate type names (deprecated):
using HMF::Coordinate::X;
using HMF::Coordinate::Y;
using HMF::Coordinate::Enum;

} // namespace geometry

#define HALBE_GEOMETRY_HASH_CLASS(CLASS)                                                           \
	template <>                                                                                    \
	struct hash<CLASS> {                                                                           \
		size_t operator()(CLASS const& t) const { return t.hash(); }                               \
	};
