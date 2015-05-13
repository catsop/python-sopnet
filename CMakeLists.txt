cmake_minimum_required(VERSION 2.8.8)
include(${CMAKE_SOURCE_DIR}/cmake/include/define_module.cmake)

#######################
# project information #
#######################

project(sopnet)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

#####################
# compiler settings #
#####################

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra -fomit-frame-pointer -fPIC -std=c++11 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DDEBUG -Wall -Wextra -fPIC -std=c++11")
if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -xHOST -ipo -no-prec-div")
  #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fast")
  #set_property(DIRECTORY PROPERTY INTERPROCEDURAL_OPTIMIZATION true)
  # Intel(R) Compiler has its own library archiver,
  # if you build libraries and do not use xiar,
  # the Intel compiler will complain about invalid
  # archives at the link phase.

  # The Intel(R) archiver is "xiar" usually
  # located in the same folder as the compiler,
  # /opt/intel/Compiler/nn/n.nn/ia32/bin/xiar
  # for example.

  FIND_PROGRAM(XIAR xiar)
  IF(XIAR)
    SET(CMAKE_AR "${XIAR}")
  ENDIF(XIAR)
  MARK_AS_ADVANCED(XIAR)

  # Intel(R) Compiler also comes with its own linker
  # which provides a number of additional benefits when
  # linking code compiled with the Intel(R) compiler.
  # Again, usually in the same place as icc itself,

  # /opt/intel/COmpiler/nn/n.nn/ia32/bin/xild per e.g

  FIND_PROGRAM(XILD xild)
  IF(XILD)
    SET(CMAKE_LINKER "${XILD}")
  ENDIF(XILD)
  MARK_AS_ADVANCED(XILD)
endif()
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release or Debug" FORCE)
endif()

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(modules)
add_subdirectory(sopnet)
add_subdirectory(pysopnet)
add_subdirectory(binaries)

###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)
