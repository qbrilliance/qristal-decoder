# Copyright Quantum Brilliance
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Dependencies
set(CORE_VERSION "1.0.0")
if(NOT DEFINED CORE_TAG)
  set(CORE_TAG v${CORE_VERSION})
endif()

# Note that you can set -DCORE_TAG=<tag> to a branch, tag or SHA if you want to download and use a specific commit of the core.

# Get version number from git tag.  Must be done before the project command is called.
find_package(Git)
if(GIT_FOUND)
  execute_process(
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND git describe --tags --abbrev=0
    OUTPUT_VARIABLE PROJECT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX REPLACE "^v(.*)" "\\1" PROJECT_VERSION "${PROJECT_VERSION}")
endif()

# Project
project(decoder
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "Quantum Brilliance Decoder"
  LANGUAGES C CXX
)

# Choose C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Adopted C++ standard.")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable/disable warnings
if(WARNINGS)
  message(STATUS "Warnings enabled")
  add_compile_options(-Wall -Wextra -Wunreachable-code -Wunused) # -Werror
else()
  add_compile_options(-w)
endif()

# Set default installation dir to the build dir.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Installation path." FORCE)
  set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OFF)
endif()
message(STATUS "Installation directory for ${PROJECT_NAME} set to ${CMAKE_INSTALL_PREFIX}.")

# Set default RPATH to the lib dir of the installation dir.
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "Search path for shared libraries to encode into binaries." FORCE)

# Work out build type
set(CMAKE_BUILD_TYPE "None" CACHE STRING "Type of build: None, Release, Debug, RelWithDebInfo or MinSizeRel." FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "None" "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Find the core
if (NOT SKIP_FIND_CORE)
  find_package(qbcore ${CORE_VERSION} QUIET)
endif()

# Try to install the core if it wasn't found.
if (NOT qbcore_FOUND)

  if (NOT INSTALL_MISSING)
    message(FATAL_ERROR "Could not find core ${CORE_VERSION}. Please set qbcore_DIR to its installed location, or pass -DINSTALL_MISSING=ON.")
  endif()

  # Include CPM for managing dependencies, and set it up to cache them in the deps folder.
  set(CPM_DOWNLOAD_VERSION 0.36.0)
  set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_LIST_DIR}/deps" CACHE PATH "Dependencies path.")
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
  if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
         https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
         ${CPM_DOWNLOAD_LOCATION}
    )
  endif()
  include(${CPM_DOWNLOAD_LOCATION})

  # Root Gitlab org. of SDK repos
  set(QB_SDK_GITLAB_ORG qbau/software-and-apps/public CACHE STRING "QB SDK Gitlab group")

  # Get the core
  CPMAddPackage(core
    VERSION ${CORE_VERSION}
    GIT_TAG ${CORE_TAG}
    GIT_SHALLOW False # Temporary (for rapid dev).
    GITLAB_REPOSITORY ${QB_SDK_GITLAB_ORG}/core
    SOURCE_DIR ${CORE_ROOT}
    OPTIONS
      "INSTALL_MISSING ON"
      "CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE}"
      "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}"
      "XACC_ROOT ${XACC_ROOT}"
      "EXATN_ROOT ${EXATN_ROOT}"
      "TNQVM_ROOT ${TNQVM_ROOT}"
      "WARNINGS ${WARNINGS}"
  )
  include(CMakeFindDependencyMacro)
  include(${CMAKE_CURRENT_BINARY_DIR}/_deps/core-build/coreDependencies.cmake)
  set(qbcore_DIR ${core_SOURCE_DIR})

endif()

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

# Install the readme
install(
  FILES README.md
  DESTINATION ${CMAKE_INSTALL_PREFIX}/docs
  RENAME README_${PROJECT_NAME}.md
)

# Add tests
add_executable(CITests_decoder
  ${qbcore_DIR}/tests/XaccInitialisedTests.cpp
  tests/SimplifiedDecoderAlgorithm.cpp
  tests/DecoderKernel.cpp
  tests/QuantumDecoderAlgorithm.cpp
)
target_link_libraries(CITests_decoder
  PRIVATE
    qb::core
)
add_test(NAME ci_tester COMMAND CITests_decoder)
set_target_properties(CITests_decoder PROPERTIES BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${XACC_ROOT}/lib")

