#include <gtest/gtest.h>

#include "scheriff/ConfigurationStates.tpp"


#define GEN_EXECUTE_PARALLEL_STUFF                                         \
	auto f1 = [&]{ p.process_event(HMF::EventSetupSynapses());      };     \
	auto f2 = [&]{ p.process_event(HMF::EventSetupAnalogReadout()); };     \
	auto f3 = [&]{ p.process_event(HMF::EventSetupL2());            };     \
	std::vector<std::function<void()> > parallel_stuff{f1, f2, f3};        \
	std::srand(::testing::UnitTest::GetInstance()->random_seed());         \
	auto execute_parallel_stuff = [&]{                                     \
		std::random_shuffle(parallel_stuff.begin(), parallel_stuff.end()); \
		for(auto & f: parallel_stuff)                                      \
			f();                                                           \
	}


TEST(ConfigurationMachine, InitialStateIsFlaggedIdle) {
	ConfigurationMachine p;
	p.start();

	ASSERT_EQ(p.current_state()[0], 0); // ugly...
	ASSERT_TRUE(p.is_flag_active<HMF::FlagIdle>());
	ASSERT_FALSE(p.tainted_hardware.invalid_state_transition);
}


TEST(ConfigurationMachine, ProcessSystemStartup) {
	ConfigurationMachine p;
	p.start();

	p.process_event(HMF::EventSystemStartup());
	ASSERT_FALSE(p.is_flag_active<HMF::FlagIdle>());
	// TODO: add better flag test
	ASSERT_FALSE(p.tainted_hardware.invalid_state_transition);
}


TEST(ConfigurationMachine, ProcessIllegalEvent) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	Scheriff p;
	p.start(true);

	ASSERT_DEATH(p.process_event(HMF::EventStartExperiment()), "no_transition");
}


TEST(ConfigurationMachine, ProcessSomeEvents) {
	Scheriff p;
	p.start();

	p.process_event(HMF::EventSystemStartup());
	p.process_event(HMF::EventResetCold());
}


TEST(ConfigurationMachine, TestStandardFlow) {
	Scheriff p;
	p.start();

	p.process_event(HMF::EventSystemStartup());
	p.process_event(HMF::EventResetCold());
	p.process_event(HMF::EventResetWarm());

	p.process_event(HMF::EventSetupFG());
	p.process_event(HMF::EventSetupL1());
	p.process_event(HMF::EventSetupL1BG());
	p.process_event(HMF::EventSetupL1DLL());

	p.process_event(HMF::EventSetupSynapses());
	p.process_event(HMF::EventSetupAnalogReadout());
	p.process_event(HMF::EventSetupL2());

	p.process_event(HMF::EventStartExperiment());
	p.process_event(HMF::EventStopExperiment());
}


TEST(ConfigurationMachine, TestInterleaved) {
	Scheriff p;

	GEN_EXECUTE_PARALLEL_STUFF;

	p.start();

	p.process_event(HMF::EventSystemStartup());
	p.process_event(HMF::EventResetCold());
	p.process_event(HMF::EventResetWarm());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupFG());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupL1());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupL1BG());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupL1DLL());

	execute_parallel_stuff();


	p.process_event(HMF::EventStartExperiment());
	p.process_event(HMF::EventStopExperiment());
}


TEST(ConfigurationMachine, TestL1BGbeforeL1Illegal) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	Scheriff p;

	GEN_EXECUTE_PARALLEL_STUFF;

	p.start(true);

	p.process_event(HMF::EventSystemStartup());
	p.process_event(HMF::EventResetCold());
	p.process_event(HMF::EventResetWarm());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupFG());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupL1BG());

	execute_parallel_stuff();

	ASSERT_DEATH(p.process_event(HMF::EventSetupL1()), "no_transition");
}


TEST(ConfigurationMachine, TestInterleavedL1BGbeforeL1Illegal1) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	Scheriff p;

	GEN_EXECUTE_PARALLEL_STUFF;

	p.start(true);

	p.process_event(HMF::EventSystemStartup());
	p.process_event(HMF::EventResetCold());
	p.process_event(HMF::EventResetWarm());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupFG());

	execute_parallel_stuff();

	p.process_event(HMF::EventSetupL1BG());

	execute_parallel_stuff();

	ASSERT_DEATH(p.process_event(HMF::EventSetupL1()), "no_transition");
}
