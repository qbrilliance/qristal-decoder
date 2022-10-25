# Copyright 2022 Quantum Brilliance
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Include CPM for managing dependencies, and set it up to cache them in the deps folder
set(CPM_DOWNLOAD_VERSION 0.36.0)
set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_LIST_DIR}/deps")
set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION}
  )
endif()
include(${CPM_DOWNLOAD_LOCATION})

# Set default installation dir to the build dir
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Installation path." FORCE)
endif()

# Get the core library
CPMAddPackage(
  NAME core
  GIT_TAG main      # Temporary (for rapid dev).
  GIT_SHALLOW False # Temporary (for rapid dev).
  GITLAB_REPOSITORY qbau/software-and-apps/core
)

# Add the core library's cmake routines to the module path, and import some
set(CMAKE_MODULE_PATH ${core_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(base_config)
include(add_xacc_plugin)
include(add_poorly_behaved_dependency)
include(add_gtest)

# Project
project(decoder
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "Quantum Brilliance Decoder"
  LANGUAGES C CXX)

# Print out the installation directory.
message(STATUS "Installation directory for ${CMAKE_PROJECT_NAME} set to ${CMAKE_INSTALL_PREFIX}.")

# Find XACC, using the path successfully validated by the core routines.
cache_install_path()
find_package(XACC REQUIRED)
reset_install_path()

# Build decoder plugin
add_xacc_plugin(decoder
  SOURCES
    src/decoder_kernel.cpp
    src/quantum_decoder.cpp
    #TODO needs a version of core that contains qb/core/circuit_builders/ry_encoding.hpp
    #src/simplified_decoder.cpp
  HEADERS
    include/decoder_kernel.hpp
    include/quantum_decoder.hpp
    include/simplified_decoder.hpp
)

# Add test
add_gtest(QuantumDecoderAlgorithm)
