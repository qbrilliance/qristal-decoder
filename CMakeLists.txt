# Copyright 2022 Quantum Brilliance
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Pre-retrieve core cmake dependency routines, for bootstrapping up to the full package.
set(CORE_TAG main)
set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_LIST_DIR}/deps" CACHE PATH "Dependencies path.")
# QB's core Gitlab repo ID (to fetch the add_dependency.cmake script) 
# Default is the public repo. 
set(QB_CORE_REPO_ID 41590729 CACHE STRING "QB SDK core Gitlab repository Id")
# Root Gitlab org. of SDK repos
set(QB_SDK_GITLAB_ORG qbau/software-and-apps/public CACHE STRING "QB SDK Gitlab group")


file(DOWNLOAD https://gitlab.com/api/v4/projects/${QB_CORE_REPO_ID}/repository/files/cmake%2Fadd_dependency%2Ecmake/raw?ref=${CORE_TAG}
     ${CPM_SOURCE_CACHE}/add_dependency.cmake
     HTTPHEADER "PRIVATE-TOKEN: $ENV{GITLAB_PRIVATE_TOKEN}"
     STATUS STATUS_LIST)
list(POP_FRONT STATUS_LIST STATUS_LIST0)
if(STATUS_LIST0)
  message(FATAL_ERROR "Failed to download core. Please set environment variable GITLAB_PRIVATE_TOKEN.")
endif()
set(CMAKE_MODULE_PATH ${CPM_SOURCE_CACHE} ${CMAKE_MODULE_PATH})
include(add_dependency)

# Load the core
add_dependency(core 0
  GIT_TAG ${CORE_TAG}
  GIT_SHALLOW False # Temporary (for rapid dev).
  GITLAB_REPOSITORY ${QB_SDK_GITLAB_ORG}/core
  SOURCE_DIR ${CORE_ROOT}
  OPTIONS
    "CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE}"
)
check_missing()

# Import other core cmake routines
set(CMAKE_MODULE_PATH ${core_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(base_config)
include(add_xacc_plugin)
include(add_poorly_behaved_dependency)

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

# Build decoder plugins
add_xacc_plugin(decoder
  SOURCES
    src/decoder_kernel.cpp
    src/quantum_decoder.cpp
    src/decoder_plugin_activator.cpp
  HEADERS
    include/qb/decoder/decoder_kernel.hpp
    include/qb/decoder/quantum_decoder.hpp
)
add_xacc_plugin(simplified_decoder
  SOURCES
    src/simplified_decoder.cpp
  HEADERS
    include/qb/decoder/simplified_decoder.hpp
)

# Install the headers
install(
  DIRECTORY include
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# Add tests
add_executable(CITests_decoder
  ${core_SOURCE_DIR}/tests/XaccInitialisedTests.cpp
  tests/SimplifiedDecoderAlgorithm.cpp
  tests/DecoderKernel.cpp
  tests/QuantumDecoderAlgorithm.cpp
)
target_link_libraries(CITests_decoder
  PRIVATE
    GTest::gtest
    qb::core_headers
    xacc::xacc
    xacc::quantum_gate
    cpr
    qb::core
)
add_test(NAME ci_tester COMMAND CITests_decoder)
set_target_properties(CITests PROPERTIES BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${XACC_ROOT}/lib")

