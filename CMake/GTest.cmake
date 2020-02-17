# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)

# Download and unpack googletest at configure time
configure_file(
  ${CMAKE_SOURCE_DIR}/CMake/GTest.in
  ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt
)
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download
)
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download
)
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

add_subdirectory(
  ${CMAKE_BINARY_DIR}/googletest-src ${CMAKE_BINARY_DIR}/googletest-build
  EXCLUDE_FROM_ALL
)

# Hide targets from "all" and put them in the UnitTests folder in MSVS
foreach(target_var gmock gtest gmock_main gtest_main)
  set_target_properties(
    ${target_var} PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "googletest"
  )
endforeach()

set(GMOCK_LIB gmock)
set(GMOCK_LIB_DEBUG gmock)
set(GMOCK_LIBRARIES optimized ${GMOCK_LIB} debug ${GMOCK_LIB_DEBUG})
set(GTEST_LIB gtest)
set(GTEST_LIB_DEBUG gtest)
set(GTEST_LIBRARIES optimized ${GTEST_LIB} debug ${GTEST_LIB_DEBUG})

find_path(
  GMOCK_INCLUDE_DIR gmock/gmock.h
  PATHS ${CMAKE_BINARY_DIR}/googletest-src/googlemock/include
  NO_DEFAULT_PATH
)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h
          PATHS ${CMAKE_BINARY_DIR}/googletest-src/googletest/include
                NO_DEFAULT_PATH
)

# handle the QUIETLY and REQUIRED arguments and set GMOCK_FOUND to TRUE if all
# listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GMOCK DEFAULT_MSG GMOCK_INCLUDE_DIR GMOCK_LIBRARIES
)
find_package_handle_standard_args(
  GTEST DEFAULT_MSG GTEST_INCLUDE_DIR GTEST_LIBRARIES
)

mark_as_advanced(GMOCK_INCLUDE_DIR GMOCK_LIB GMOCK_LIB_DEBUG)
mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIB GTEST_LIB_DEBUG)
include(GoogleTest)
