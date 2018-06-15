#include <boost/program_options.hpp>
#include <gtest/gtest.h>
#include "logging_ctrl.h"

#include "logger.h"

#include "ess-test-util.h"

int main(int argc, char *argv[])
{
	//logger_log_to_cout(log4cxx::Level::getInfo());
    logger_log_to_file("test-log.txt", log4cxx::Level::getDebug() );
    testing::InitGoogleTest(&argc, argv);

	//connection info from test-util.h
	ESSTestEnviroment * g_env = new ESSTestEnviroment;
	::testing::AddGlobalTestEnvironment(g_env);
	return RUN_ALL_TESTS();
}
