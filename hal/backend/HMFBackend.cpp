#include "hal/backend/HMFBackend.h"

#include <iostream>

#include "hal/macro_HALbe.h"
#include "logger.h"
#include "git_version.h"
#include "halbe_git_version.h"

namespace HMF {

void Debug::change_loglevel(int level) {
	std::cout << "logger level is " << level << std::endl;
	Logger::instance("HALbe", level, "");
}

// FIXME: GETTER :)?
std::string Debug::getHalbeGitVersion()
{
	return HALBE_GIT_VERSION;
}

std::string Debug::getHicannSystemGitVersion()
{
	return GIT_VERSION;
}

} //namespace HMF
