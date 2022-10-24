# Copyright 2022 Quantum Brilliance
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Time of running cmake
string(TIMESTAMP TODAY)

# Get version number from git tag
find_package(Git)

if(GIT_FOUND)
  execute_process(
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND git describe --tags --abbrev=0
    OUTPUT_VARIABLE FULL_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${FULL_VERSION}")
  string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${FULL_VERSION}")
  string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_REVISION "${FULL_VERSION}")
  string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-*(.*)" "\\1" VERSION_PATCH "${FULL_VERSION}")
endif()

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")
if(VERSION_PATCH)
  set(PROJECT_VERSION "${PROJECT_VERSION}.${VERSION_PATCH}")
  set(SHORT_VERSION "Version: ${VERSION_PATCH} Build time ${TODAY}")
else()
  set(SHORT_VERSION "Version: ${PROJECT_VERSION} Build time ${TODAY}")
endif()

# Project
project(qbdecoder
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "Quantum Brilliance Decoder"
  LANGUAGES C CXX)

# Set default installation dir to the build dir
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "..." FORCE)
endif()
message("-- Installation directory set to ${CMAKE_INSTALL_PREFIX}.")

# Set default RPATH to the lib dir of the installation dir
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "..." FORCE)

# Include CPM for managing dependencies, and set it up to cache them in the deps folder
set(CPM_DOWNLOAD_VERSION 0.36.0)
set(CPM_SOURCE_CACHE "${PROJECT_SOURCE_DIR}/deps")
set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION}
  )
endif()
include(${CPM_DOWNLOAD_LOCATION})

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
include(add_gtest)

# Find XACC, using the path successfully validated by the core routines.
find_package(XACC REQUIRED)

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
