add_executable ( data_frame_test data_frame_test.cpp )
target_include_directories ( data_frame_test PRIVATE ".." )
target_link_libraries( data_frame_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )
