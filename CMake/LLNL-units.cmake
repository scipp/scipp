# Prevent overriding the parent project's compiler/linker settings on Windows
set(llnl_units_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)

# Download and unpack llnl-units at configure time
configure_file(
  ${CMAKE_SOURCE_DIR}/CMake/LLNL-units.in
  ${CMAKE_BINARY_DIR}/llnl-units-download/CMakeLists.txt
)
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/llnl-units-download
)
if(result)
  message(FATAL_ERROR "CMake step for llnl-units failed: ${result}")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/llnl-units-download
)
if(result)
  message(FATAL_ERROR "Build step for llnl-units failed: ${result}")
endif()

set(UNITS_NAMESPACE "llnl::units" CACHE STRING "" FORCE)
add_subdirectory(
  ${CMAKE_BINARY_DIR}/llnl-units-src ${CMAKE_BINARY_DIR}/llnl-units-build
  EXCLUDE_FROM_ALL
)

set_target_properties(units-static PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
