#ifndef TESTS_H__
#define TESTS_H__

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>
#include <boost/filesystem.hpp>
#include <util/Logger.h>

using namespace boost::unit_test;

#define ADD_TEST_CASE(function) void function();         ts->add(BOOST_TEST_CASE(&function));
#define ADD_TEST_SUITE(suite)   void suite(test_suite&); suite(*ts);

#define BEGIN_TEST_MODULE(name) bool name() { \
                                  test_suite* ts = BOOST_TEST_SUITE(#name);

#define END_TEST_MODULE()         framework::master_test_suite().add(ts); \
                                  return true; \
                                }

#define BEGIN_TEST_SUITE(name) void name(test_suite& parent) { \
                                 test_suite* ts = BOOST_TEST_SUITE(#name);

#define END_TEST_SUITE()         parent.add(ts); \
                               }

boost::filesystem::path dir_of(const char* filename);

extern logger::LogChannel testslog;

#endif // TESTS_H__
