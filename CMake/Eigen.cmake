# Download and unpack Eigen at configure time
configure_file(${CMAKE_SOURCE_DIR}/CMake/Eigen.in
  ${CMAKE_BINARY_DIR}/Eigen-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Eigen-download )

execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Eigen-download )
