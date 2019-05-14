# Download and unpack boost at configure time
configure_file(${CMAKE_SOURCE_DIR}/CMake/boost.in
               ${CMAKE_BINARY_DIR}/boost-download/CMakeLists.txt)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/boost-download)
if(result)
  message(FATAL_ERROR "CMake step for boost failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/boost-download)
if(result)
  message(FATAL_ERROR "Build step for boost failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --install .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/boost-download)
if(result)
  message(FATAL_ERROR "Install step for boost failed: ${result}")
endif()

set(BOOST_ROOT "${CMAKE_BINARY_DIR}/boost/include" CACHE PATH "Path to boost")
