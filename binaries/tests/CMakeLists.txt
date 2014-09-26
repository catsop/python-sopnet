#define_module(catsop_binary_test OBJECT LINKS util signals pipeline boost imageprocessing sopnet_all INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/.. )
define_module(test_postgres_stores BINARY SOURCES test_postgres_stores.cpp LINKS sopnet_all sopnet_catmaid)
