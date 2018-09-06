# Download and unpack gsl at configure time
configure_file(${CMAKE_SOURCE_DIR}/CMake/gsl.in
               ${CMAKE_BINARY_DIR}/gsl-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/gsl-download )

execute_process(COMMAND ${CMAKE_COMMAND} --build .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/gsl-download )
