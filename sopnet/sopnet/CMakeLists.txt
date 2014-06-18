define_module(sopnet_all OBJECT LINKS sopnet_external sopnet_inference pipeline gui imageprocessing boost hdf5 gurobi cplex INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
