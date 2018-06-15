#include <gtest/gtest.h>

#include "hwtest.h"
#include "CommandLineArgs.h"

#include "logging_ctrl.h"

class HICANNBackendTestEnvironment : public ::testing::Environment {
public:
	explicit HICANNBackendTestEnvironment() = default;

	void set(const CommandLineArgs & conn ) { g_conn = conn; }
};

int main(int argc, char *argv[])
{
	logger_default_config();

	testing::InitGoogleTest(&argc, argv);

	HICANNBackendTestEnvironment * g_env = new HICANNBackendTestEnvironment;
	g_env->set(CommandLineArgs::parse(argc, argv));
	::testing::AddGlobalTestEnvironment(g_env);

	return RUN_ALL_TESTS();
}
