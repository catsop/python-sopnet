if (HAVE_PostgreSQL)
	define_module(test_postgres_stores BINARY SOURCES test_postgres_stores.cpp LINKS sopnet_core sopnet_blockwise)
endif()
