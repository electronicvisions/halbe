#pragma once
/**
 * @file
 * @mainpage
 * @author Sebastian Jeltsch <sjeltsch@kip.uni-heidelberg.de> (Coordinate System & Implementation)
 * @author Christoph Koke    <koke@kip.uni-heidelberg.de>     (Implementation)
 * @author Alexander Kononov <kononov@kip.uni-heidelberg.de>  (Implementation)
 * @author Eric Müller       <mueller@kip.uni-heidelberg.de>  (Implementation & Handles)
 * @date 2012-2013
 *
 * This file provides the description (or includes other files) of the low level
 * HAL interface for the HICANN chip, DNC and FPGA functionality.
 *
 * Design Principle:
 * =================
 * - Each function itself is stateless - no statics, no globals!
 * - We use "handles" to represent the communication stream.
 * - The communication stream is encapsulated, access to shared data via atomic
 *   operations.
 * - Data needs to be stored and managed on a higher level in the hierarchy
 *   (i.e. some stateful HAL => StHAL).
 *
 * The Coordinate System
 * =====================
 *
 * Representing the overall possible connectivity across borders of individual
 * chip would require huge connectivity matrices which is neither practicable
 * nor modular. Therefore, implicit knowledge about connection possibilities is
 * still required. Since routing is a global operation and the most complex task
 * the major goal was to ease coding by making the L1 topology addressing
 * consistent with regards to a Cartesian coordinate system. Furthermore,
 * addressing all directly connected components by their corresponding L1
 * address.
 *
 * Special Coordinates:
 * -------------------
 * * HWire                : horizontal bus wires addresses:
 *    * range from 0 (left) to 223 (right) for synapsedriver switch rows
 *    * range from 0 (left) to 63 (right)  for crossbar switch rows
 * * Side(SideHorizontal) : either LEFT or RIGHT
 * * SideVertical         : either TOP or BOTTOM
 */

#include "hal/backend/DNCBackend.h"
#include "hal/backend/HICANNBackend.h"
#include "hal/backend/FPGABackend.h"
#include "hal/backend/ADCBackend.h"

namespace HMF {

namespace Debug {
	void change_loglevel(int level);

	std::string getHalbeGitVersion();
	std::string getHicannSystemGitVersion();
}

} // namespace HMF
