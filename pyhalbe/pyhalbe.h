#pragma once

// MPL redefines
#include "scheriff/defines.h"

// Backend (includes Containers)
#include "hal/backend/ADCBackend.h"
#include "hal/backend/HMFBackend.h"
#include "hal/backend/HICANNBackend.h"

// Handles
#include "hal/Handle/ADC.h"
#include "hal/Handle/ADCHw.h"
#include "hal/Handle/ADCRemoteHw.h"
#include "hal/Handle/HICANN.h"
#include "hal/Handle/HICANNDump.h"
#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/FPGA.h"
#include "hal/Handle/FPGADump.h"
#include "hal/Handle/FPGAHw.h"
#include "hal/Handle/HMFRun.h"
#include "hal/Handle/ADCDump.h"

#if defined(HAVE_ESS)
#include "hal/Handle/ADCEss.h"
#include "hal/Handle/FPGAEss.h"
#include "hal/Handle/HICANNEss.h"
#endif

// Stuff
#include "hal/HMFUtil.h"
#include "hal/Coordinate/FormatHelper.h"
