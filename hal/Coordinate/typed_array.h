#pragma once

#include "halco/common/typed_array.h"

namespace HMF {
namespace Coordinate {

using halco::common::typed_array;

#ifndef PYPLUSPLUS
namespace typed_array_enum_support {

using halco::common::typed_array_enum_support::limits;
}
#endif

} // namespace Coordinate
} // namespace HMF
