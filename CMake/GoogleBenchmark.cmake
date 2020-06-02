# Download and unpack googlebenchmark at configure time

set(BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL "" FORCE
)

configure_file(
  ${CMAKE_SOURCE_DIR}/CMake/GoogleBenchmark.in
  ${CMAKE_BINARY_DIR}/googlebenchmark-download/CMakeLists.txt
)

execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googlebenchmark-download
                    RESULTS_VARIABLE result
)

if(result)
  message(FATAL_ERROR "CMake step for GoogleBenchmark failed: ${result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googlebenchmark-download
                    RESULTS_VARIABLE result
)
if(result)
  message(FATAL_ERROR "Build step for GoogleBenchmark failed: ${result}")
endif()

add_subdirectory(
  ${CMAKE_BINARY_DIR}/googlebenchmark-src
  ${CMAKE_BINARY_DIR}/googlebenchmark-build EXCLUDE_FROM_ALL
)
