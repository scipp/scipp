# Download and unpack Eigen at configure time
configure_file(
  ${CMAKE_SOURCE_DIR}/CMake/Eigen.in
  ${CMAKE_BINARY_DIR}/Eigen-download/CMakeLists.txt
)

execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Eigen-download RESULTS_VARIABLE result
)

if(result)
  message(FATAL_ERROR "CMake step for Eigen failed: ${result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Eigen-download RESULTS_VARIABLE result
)

if(result)
  message(FATAL_ERROR "Build step for Eigen failed: ${result}")
endif()
