add_subdirectory(tests)
define_module(coresolvertest     BINARY SOURCES coresolvertest.cpp     LINKS sopnet_blockwise sopnet_core)
