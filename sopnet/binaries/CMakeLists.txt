add_subdirectory(tests)
define_module(coresolvertest     BINARY SOURCES coresolvertest.cpp     LINKS sopnet_blockwise sopnet_core)
define_module(extract_local_labels     BINARY SOURCES extract_local_labels.cpp     LINKS sopnet_blockwise sopnet_core)
