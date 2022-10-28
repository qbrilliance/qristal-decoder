# Copyright 2022 Quantum Brilliance
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Pre-retrieve core cmake dependency routines, for bootstrapping up to the full package.
set(CORE_TAG main)
file(DOWNLOAD https://gitlab.com/api/v4/projects/39963594/repository/files/cmake%2Fadd_dependency%2Ecmake/raw?ref=${CORE_TAG}
     ${CMAKE_CURRENT_LIST_DIR}/deps/add_dependency.cmake
     HTTPHEADER "PRIVATE-TOKEN: $ENV{GITLAB_PRIVATE_TOKEN}"
     STATUS STATUS_LIST)
list(POP_FRONT STATUS_LIST STATUS_LIST0)
if(STATUS_LIST0)
  message(FATAL_ERROR "Failed to download core. Please set environment variable GITLAB_PRIVATE_TOKEN.")
endif()
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/deps ${CMAKE_MODULE_PATH})
include(add_dependency)

# Load the core
add_dependency(core 0
  GIT_TAG ${CORE_TAG}
  GIT_SHALLOW False # Temporary (for rapid dev).
  GITLAB_REPOSITORY qbau/software-and-apps/core
)
check_missing()

# Project
project(decoder
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "Quantum Brilliance Decoder"
  LANGUAGES C CXX)

# Import other core cmake routines
set(CMAKE_MODULE_PATH ${core_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(base_config)
include(add_xacc_plugin)
include(add_poorly_behaved_dependency)
include(add_gtest)

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
