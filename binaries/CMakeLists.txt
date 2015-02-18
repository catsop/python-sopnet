add_subdirectory(tests)
define_module(coresolvertest     BINARY SOURCES coresolvertest.cpp     LINKS catsop_binary_test sopnet_catmaid sopnet_all)
