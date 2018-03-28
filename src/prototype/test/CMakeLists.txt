add_executable ( compile_time_test compile_time_test.cpp )
target_include_directories ( compile_time_test PRIVATE ".." )

add_executable ( prototype_test test.cpp )
target_include_directories ( prototype_test PRIVATE ".." )
target_link_libraries( prototype_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )
