#define CONCAT(arg1, arg2)   CONCAT1(arg1, arg2)
#define CONCAT1(arg1, arg2)  CONCAT2(arg1, arg2)
#define CONCAT2(arg1, arg2)  arg1##arg2

#define MY_VA_NUM_ARGS(...) \
	MY_VA_NUM_ARGS_IMPL(EMPTYZERO, ##__VA_ARGS__, \
	20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define MY_VA_NUM_ARGS_IMPL(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,N,...) N

#define MYEMPTY2(x)
#define MYEMPTY(...) MYEMPTY2(MY_VA_NUM_ARGS(__VA_ARGS__))

#define EVERYSECOND_0(...) MYEMPTY(__VA_ARGS__)
#define EVERYSECOND_2(a, b) b
#define EVERYSECOND_4(a, b, ...)  b, EVERYSECOND_2(__VA_ARGS__)
#define EVERYSECOND_6(a, b, ...)  b, EVERYSECOND_4(__VA_ARGS__)
#define EVERYSECOND_8(a, b, ...)  b, EVERYSECOND_6(__VA_ARGS__)
#define EVERYSECOND_10(a, b, ...) b, EVERYSECOND_8(__VA_ARGS__)
#define EVERYSECOND_12(a, b, ...) b, EVERYSECOND_10(__VA_ARGS__)
#define EVERYSECOND_14(a, b, ...) b, EVERYSECOND_12(__VA_ARGS__)
#define EVERYSECOND_16(a, b, ...) b, EVERYSECOND_14(__VA_ARGS__)
#define EVERYSECOND_18(a, b, ...) b, EVERYSECOND_16(__VA_ARGS__)
#define EVERYSECOND_20(a, b, ...) b, EVERYSECOND_18(__VA_ARGS__)

#define EVERYSECOND_1( a)
#define EVERYSECOND_3( a, b, c) b
#define EVERYSECOND_5( a, b, c, ...) b, EVERYSECOND_3( c, __VA_ARGS__)
#define EVERYSECOND_7( a, b, c, ...) b, EVERYSECOND_5( c, __VA_ARGS__)
#define EVERYSECOND_9( a, b, c, ...) b, EVERYSECOND_7( c, __VA_ARGS__)
#define EVERYSECOND_11(a, b, c, ...) b, EVERYSECOND_9( c, __VA_ARGS__)
#define EVERYSECOND_13(a, b, c, ...) b, EVERYSECOND_11(c, __VA_ARGS__)
#define EVERYSECOND_15(a, b, c, ...) b, EVERYSECOND_13(c, __VA_ARGS__)
#define EVERYSECOND_17(a, b, c, ...) b, EVERYSECOND_15(c, __VA_ARGS__)
#define EVERYSECOND_19(a, b, c, ...) b, EVERYSECOND_17(c, __VA_ARGS__)

#define EVERYSECOND_(N, ...) CONCAT(EVERYSECOND_, N)(__VA_ARGS__)
#define EVERYSECOND(...) EVERYSECOND_( MY_VA_NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#define EVERYSECOND_DROP_LAST_0(...) MYEMPTY(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_2(a, b)
#define EVERYSECOND_DROP_LAST_4(a, b, ...)  b
#define EVERYSECOND_DROP_LAST_6(a, b, ...)  b, EVERYSECOND_DROP_LAST_4(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_8(a, b, ...)  b, EVERYSECOND_DROP_LAST_6(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_10(a, b, ...) b, EVERYSECOND_DROP_LAST_8(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_12(a, b, ...) b, EVERYSECOND_DROP_LAST_10(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_14(a, b, ...) b, EVERYSECOND_DROP_LAST_12(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_16(a, b, ...) b, EVERYSECOND_DROP_LAST_14(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_18(a, b, ...) b, EVERYSECOND_DROP_LAST_16(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST_20(a, b, ...) b, EVERYSECOND_DROP_LAST_18(__VA_ARGS__)

#define EVERYSECOND_DROP_LAST_1( a)
#define EVERYSECOND_DROP_LAST_3( a, b, c) b
#define EVERYSECOND_DROP_LAST_5( a, b, c, ...) b, EVERYSECOND_DROP_LAST_3( c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_7( a, b, c, ...) b, EVERYSECOND_DROP_LAST_5( c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_9( a, b, c, ...) b, EVERYSECOND_DROP_LAST_7( c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_11(a, b, c, ...) b, EVERYSECOND_DROP_LAST_9( c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_13(a, b, c, ...) b, EVERYSECOND_DROP_LAST_11(c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_15(a, b, c, ...) b, EVERYSECOND_DROP_LAST_13(c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_17(a, b, c, ...) b, EVERYSECOND_DROP_LAST_15(c, __VA_ARGS__)
#define EVERYSECOND_DROP_LAST_19(a, b, c, ...) b, EVERYSECOND_DROP_LAST_17(c, __VA_ARGS__)

#define EVERYSECOND_DROP_LAST_(N, ...) CONCAT(EVERYSECOND_DROP_LAST_, N)(__VA_ARGS__)
#define EVERYSECOND_DROP_LAST(...) EVERYSECOND_DROP_LAST_( MY_VA_NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#define EVERYTWO_0(...) MYEMPTY(__VA_ARGS__)
#define EVERYTWO_2(a, b) a b
#define EVERYTWO_4(a, b, ...)  a b, EVERYTWO_2(__VA_ARGS__)
#define EVERYTWO_6(a, b, ...)  a b, EVERYTWO_4(__VA_ARGS__)
#define EVERYTWO_8(a, b, ...)  a b, EVERYTWO_6(__VA_ARGS__)
#define EVERYTWO_10(a, b, ...) a b, EVERYTWO_8(__VA_ARGS__)
#define EVERYTWO_12(a, b, ...) a b, EVERYTWO_10(__VA_ARGS__)
#define EVERYTWO_14(a, b, ...) a b, EVERYTWO_12(__VA_ARGS__)
#define EVERYTWO_16(a, b, ...) a b, EVERYTWO_14(__VA_ARGS__)
#define EVERYTWO_18(a, b, ...) a b, EVERYTWO_16(__VA_ARGS__)
#define EVERYTWO_20(a, b, ...) a b, EVERYTWO_18(__VA_ARGS__)

#define EVERYTWO_1(a)
#define EVERYTWO_3(a, b, c)       a b
#define EVERYTWO_5(a, b, c, ...)  a b, EVERYTWO_3(c, __VA_ARGS__)
#define EVERYTWO_7(a, b, c, ...)  a b, EVERYTWO_5(c, __VA_ARGS__)
#define EVERYTWO_9(a, b, c, ...)  a b, EVERYTWO_7(c, __VA_ARGS__)
#define EVERYTWO_11(a, b, c, ...) a b, EVERYTWO_9(c, __VA_ARGS__)
#define EVERYTWO_13(a, b, c, ...) a b, EVERYTWO_11(c, __VA_ARGS__)
#define EVERYTWO_15(a, b, c, ...) a b, EVERYTWO_13(c, __VA_ARGS__)
#define EVERYTWO_17(a, b, c, ...) a b, EVERYTWO_15(c, __VA_ARGS__)
#define EVERYTWO_19(a, b, c, ...) a b, EVERYTWO_17(c, __VA_ARGS__)

#define EVERYTWO_(N, ...) CONCAT(EVERYTWO_, N)(__VA_ARGS__)
#define EVERYTWO(...) EVERYTWO_( MY_VA_NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#include <boost/preprocessor/variadic/elem.hpp>

#define MYFIRST(...)  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)
#define MYSECOND(...) BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.h>

#define COMPARE_EQUAL_OP(r, OTHER, VAR) (VAR == OTHER . VAR) &&
#define COMPARE_EQUAL(OTHER, ...) BOOST_PP_SEQ_FOR_EACH(COMPARE_EQUAL_OP, OTHER ,BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) true

// TODO: future => hide all non-api stuff :)
#define HALBE_API      __attribute__ ((visibility ("default")))
#define HALBE_NON_API  __attribute__ ((visibility ("hidden")))
