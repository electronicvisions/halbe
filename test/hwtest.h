#include <gtest/gtest.h>

#include <algorithm>
#include <boost/filesystem.hpp>


#include "hal/backend/HMFBackend.h"

// runtime handles...
#include "hal/Handle/HMFRun.h"
#include "hal/Handle/HICANN.h"
#include "hal/Handle/FPGAHw.h"
#include "hal/Handle/FPGADump.h"
#ifdef HAVE_ESS
#include "hal/Handle/FPGAEss.h"
#endif

#include "hal/DNCContainer.h"
#include "hal/HMFCommon.h"

#include "CommandLineArgs.h"

extern CommandLineArgs g_conn;

struct RedirectStdout;

struct HWTestHandleBase
{
	facets::ReticleControl * get_reticle() { return nullptr; }
	bool has_getter() { return true; }
	uint8_t jtag_addr() { throw std::runtime_error("Don't get here..."); }

	void InstanceSetUp() {};
	void InstanceTearDown() {};
};

template <typename FPGA>
struct HWTestHandle;

template<>
struct HWTestHandle< ::HMF::Handle::FPGAHw > : public HWTestHandleBase
{
protected:
	HWTestHandle()
	    : handle(::HMF::Handle::FPGAHw::HandleParameter{
	          g_conn.f, g_conn.fpga_ip, g_conn.d, g_conn.available_hicanns,
	          g_conn.available_hicanns, g_conn.available_hicanns, g_conn.setup, g_conn.pmu_ip, g_conn.jtag_freq}),
	      hhandle(*handle.get(g_conn.d, g_conn.h))
	{}

	void InstanceSetUp();
	void InstanceTearDown();

	facets::ReticleControl * get_reticle();
	uint8_t jtag_addr();

	HMF::Handle::FPGAHw     handle;
	HMF::Handle::HICANNHw & hhandle;

private:
	HMF::Coordinate::IPv4 ip;
};

template<>
struct HWTestHandle< ::HMF::Handle::FPGADump > : public HWTestHandleBase
{
protected:
	HWTestHandle() :
		dumper(new ::HMF::Handle::Dump()),
		handle(dumper, g_conn.f),
		hhandle(*handle.get(g_conn.d, g_conn.h))
	{
	}

	void InstanceSetUp() {
		using namespace boost::filesystem;

		const ::testing::TestInfo* const test_info =
			  ::testing::UnitTest::GetInstance()->current_test_info();
		std::string full_test_name = std::string(test_info->name()) + "." + test_info->test_case_name();
		std::replace(full_test_name.begin(), full_test_name.end(), '/', '_');

		create_directory(dump_file);
		path basename = dump_file / full_test_name;

		dumper->enableXmlDump(basename.native() + ".xml", true);
	}

	void InstanceTearDown() {};

	bool has_getter() { return false; }

	boost::shared_ptr< ::HMF::Handle::Dump > dumper;
	HMF::Handle::FPGADump     handle;
	HMF::Handle::HICANNDump & hhandle;

	static boost::filesystem::path dump_file;
};


#ifdef HAVE_ESS
template<>
struct HWTestHandle< ::HMF::Handle::FPGAEss > : public HWTestHandleBase
{
protected:
	HWTestHandle() :
		ess(new ::HMF::Handle::Ess()),
		handle(g_conn.f,
			   ess,
			   std::vector<HMF::Coordinate::HICANNOnWafer>{getHICANNOnWafer()}),
		hhandle(*handle.get(g_conn.d, g_conn.h))
	{
	}

	void InstanceSetUp() {
	
	}

	void InstanceTearDown() {};

	bool has_getter() { return true; }

	static HMF::Coordinate::HICANNOnWafer getHICANNOnWafer()
	{
		HMF::Coordinate::DNCOnWafer dnc = g_conn.d.toDNCOnWafer(g_conn.f);
		return g_conn.h.toHICANNOnWafer(dnc);
	}

	boost::shared_ptr< ::HMF::Handle::Ess > ess;
	HMF::Handle::FPGAEss     handle;
	HMF::Handle::HICANNEss & hhandle;
};
#endif


template <typename T>
struct TypedHWTest :
	public HWTestHandle<T>,
	public ::testing::Test
{
private:
	typedef HWTestHandle<T> base_t;
protected:
	TypedHWTest() :
		base_t(),
		f(base_t::handle),
		h(base_t::hhandle),
		d(h.coordinate().toDNCOnFPGA()),
		hc(h.coordinate().toHICANNOnDNC())
	{}

	virtual void SetUp() {
		base_t::InstanceSetUp();
		if (g_conn.use_scheriff)
			f.enableScheriff();
		::HMF::FPGA::reset(f);
	}

	virtual void TearDown() {
		base_t::InstanceTearDown();
	}

	HMF::Handle::FPGA &                f;
	HMF::Handle::HICANN &              h;
	const HMF::Coordinate::DNCOnFPGA   d;
	const HMF::Coordinate::HICANNOnDNC hc;
};

typedef ::testing::Types<
	::HMF::Handle::FPGAHw,
	::HMF::Handle::FPGADump
	// TODO add ::HMF::Handle::FPGAESS
> HandleTypes;

typedef TypedHWTest< ::HMF::Handle::FPGAHw > HWTest;

// Redefine gtest macros, that will be disabled for handles without useful getters
#define __ASSERT_HELPER(ASSERT, ...) \
	if (!this->has_getter()) {} else ASSERT(__VA_ARGS__)

#define EXPECT_GETTER_THROW(statement, expected_exception) __ASSERT_HELPER(EXPECT_THROW, statement, expected_exception)
#define EXPECT_GETTER_NO_THROW(statement)                  __ASSERT_HELPER(EXPECT_NO_THROW, statement)
#define EXPECT_GETTER_ANY_THROW(statement)                 __ASSERT_HELPER(EXPECT_ANY_THROW, statement)
#define EXPECT_GETTER_TRUE(condition)                      __ASSERT_HELPER(EXPECT_TRUE, condition)
#define EXPECT_GETTER_FALSE(condition)                     __ASSERT_HELPER(EXPECT_FALSE, condition)
#define EXPECT_GETTER_EQ(expected, actual)                 __ASSERT_HELPER(EXPECT_EQ, expected, actual)
#define EXPECT_GETTER_NE(expected, actual)                 __ASSERT_HELPER(EXPECT_NE, expected, actual)
#define EXPECT_GETTER_LE(val1, val2)                       __ASSERT_HELPER(EXPECT_LE, val1, val2)
#define EXPECT_GETTER_LT(val1, val2)                       __ASSERT_HELPER(EXPECT_LT, val1, val2)
#define EXPECT_GETTER_GE(val1, val2)                       __ASSERT_HELPER(EXPECT_GE, val1, val2)
#define EXPECT_GETTER_GT(val1, val2)                       __ASSERT_HELPER(EXPECT_GT, val1, val2)
#define EXPECT_GETTER_STREQ(expected, actual)              __ASSERT_HELPER(EXPECT_STREQ, expected, actual)
#define EXPECT_GETTER_STRNE(s1, s2)                        __ASSERT_HELPER(EXPECT_STRNE, s1, s2)
#define EXPECT_GETTER_STRCASEEQ(expected, actual)          __ASSERT_HELPER(EXPECT_STRCASEEQ, expected, actual)
#define EXPECT_GETTER_STRCASENE(s1, s2)                    __ASSERT_HELPER(EXPECT_STRCASENE, s1, s2)
#define EXPECT_GETTER_FLOAT_EQ(expected, actual)           __ASSERT_HELPER(EXPECT_FLOAT_EQ, expected, actual)
#define EXPECT_GETTER_DOUBLE_EQ(expected, actual)          __ASSERT_HELPER(EXPECT_DOUBLE_EQ, expected, actual)
#define EXPECT_GETTER_NEAR(val1, val2, abs_error)          __ASSERT_HELPER(EXPECT_NEAR, val1, val2, abs_error)

#define ASSERT_GETTER_THROW(statement, expected_exception) __ASSERT_HELPER(ASSERT_THROW, statement, expected_exception)
#define ASSERT_GETTER_NO_THROW(statement)                  __ASSERT_HELPER(ASSERT_NO_THROW, statement)
#define ASSERT_GETTER_ANY_THROW(statement)                 __ASSERT_HELPER(ASSERT_ANY_THROW, statement)
#define ASSERT_GETTER_TRUE(condition)                      __ASSERT_HELPER(ASSERT_TRUE, condition)
#define ASSERT_GETTER_FALSE(condition)                     __ASSERT_HELPER(ASSERT_FALSE, condition)
#define ASSERT_GETTER_EQ(expected, actual)                 __ASSERT_HELPER(ASSERT_EQ, expected, actual)
#define ASSERT_GETTER_NE(expected, actual)                 __ASSERT_HELPER(ASSERT_NE, expected, actual)
#define ASSERT_GETTER_LE(val1, val2)                       __ASSERT_HELPER(ASSERT_LE, val1, val2)
#define ASSERT_GETTER_LT(val1, val2)                       __ASSERT_HELPER(ASSERT_LT, val1, val2)
#define ASSERT_GETTER_GE(val1, val2)                       __ASSERT_HELPER(ASSERT_GE, val1, val2)
#define ASSERT_GETTER_GT(val1, val2)                       __ASSERT_HELPER(ASSERT_GT, val1, val2)
#define ASSERT_GETTER_STREQ(expected, actual)              __ASSERT_HELPER(ASSERT_STREQ, expected, actual)
#define ASSERT_GETTER_STRNE(s1, s2)                        __ASSERT_HELPER(ASSERT_STRNE, s1, s2)
#define ASSERT_GETTER_STRCASEEQ(expected, actual)          __ASSERT_HELPER(ASSERT_STRCASEEQ, expected, actual)
#define ASSERT_GETTER_STRCASENE(s1, s2)                    __ASSERT_HELPER(ASSERT_STRCASENE, s1, s2)
#define ASSERT_GETTER_FLOAT_EQ(expected, actual)           __ASSERT_HELPER(ASSERT_FLOAT_EQ, expected, actual)
#define ASSERT_GETTER_DOUBLE_EQ(expected, actual)          __ASSERT_HELPER(ASSERT_DOUBLE_EQ, expected, actual)
#define ASSERT_GETTER_NEAR(val1, val2, abs_error)          __ASSERT_HELPER(ASSERT_NEAR, val1, val2, abs_error)
#define ASSERT_GETTER_NO_FATAL_FAILURE(statement)           __ASSERT_HELPER(ASSERT_NO_FATAL_FAILURE, statement)

namespace {

class ConstantSequence
{
public:
    ConstantSequence(int x) : x_(x) {}
    int operator() () { return x_; }
private:
    int x_;
};

template <typename T>
class IncrementingSequence
{
public:
    // Constructor, just set counter to 0
    IncrementingSequence(int max) : i_(0), max_i_(max) {}
    // Return an incrementing number
    T operator() ()
    {
		if (i_ == max_i_+1) i_ = 0; //reset value
		return T(i_++);
	}
private:
    int i_;
    int max_i_;
};


} // end anonymous
