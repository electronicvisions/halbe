#pragma once

#include "halco/common/relations.h"

namespace HMF {
namespace Coordinate {

using halco::common::SideHorizontal;
using halco::common::left;
using halco::common::right;
using halco::common::Side;
using halco::common::SideVertical;
using halco::common::top;
using halco::common::bottom;
using halco::common::Orientation;
using halco::common::Direction;
using halco::common::Parity;
using halco::common::north;
using halco::common::east;
using halco::common::south;
using halco::common::west;
using halco::common::horizontal;
using halco::common::vertical;

} // Coordinate
} // HMF

namespace geometry {

// Legacy coordinate type names (deprecated):
using HMF::Coordinate::top;
using HMF::Coordinate::bottom;
using HMF::Coordinate::left;
using HMF::Coordinate::right;
using HMF::Coordinate::Side;
using HMF::Coordinate::SideHorizontal;
using HMF::Coordinate::SideVertical;

} // namespace geometry
