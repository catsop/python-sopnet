add_subdirectory(external)
define_module(sopnet_core OBJECT LINKS sopnet_core_external pipeline imageprocessing solvers boost hdf5 INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
