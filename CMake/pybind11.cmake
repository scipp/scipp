# Download and unpack pybind11 at configure time
configure_file(${CMAKE_SOURCE_DIR}/CMake/pybind11.in
               ${CMAKE_BINARY_DIR}/pybind11-download/CMakeLists.txt)

       execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR} -DPYTHON_EXCUTABLE=${PYTHON_EXECUTABLE}" .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pybind11-download )

execute_process(COMMAND ${CMAKE_COMMAND} --build .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pybind11-download )

add_subdirectory(${CMAKE_BINARY_DIR}/pybind11-src
                 ${CMAKE_BINARY_DIR}/pybind11-src)
