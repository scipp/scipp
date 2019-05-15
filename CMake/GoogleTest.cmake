# Download and unpack googletest at configure time
configure_file(${CMAKE_SOURCE_DIR}/CMake/GoogleTest.in
               ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --install .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if(result)
  message(FATAL_ERROR "Install step for googletest failed: ${result}")
endif()

set(GTEST_ROOT "${CMAKE_BINARY_DIR}/googletest" CACHE PATH "Path to googletest")
find_package(GTest REQUIRED)
