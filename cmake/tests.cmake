include(${CMAKE_CURRENT_LIST_DIR}/../cmake/cpm.cmake)

CPMAddPackage(gtest
  GIT_TAG v1.17.0
  GIT_REPOSITORY https://github.com/google/googletest.git
  OPTIONS
    "INSTALL_GTEST OFF"
)

# XaccInitialisedTests.cpp is in a different core location in the source vs install.
# Add a test to check which one to use.

# Default to core install location
set(XACC_TEST_DIR ${qristal_core_DIR}/tests)
if(EXISTS ${qristal_core_DIR}/tests/misc_cpp)
  # Use core source location instead
  set(XACC_TEST_DIR ${qristal_core_DIR}/tests/misc_cpp)
endif()

# Add tests
add_executable(CITests_decoder
  ${XACC_TEST_DIR}/XaccInitialisedTests.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../tests/SimplifiedDecoderAlgorithm.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../tests/DecoderKernel.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../tests/QuantumDecoderAlgorithm.cpp
)
target_link_libraries(CITests_decoder
  PRIVATE
    qristal::core
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)
add_test(NAME ci_tester COMMAND CITests_decoder)
