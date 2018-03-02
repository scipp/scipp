add_executable ( dataset_test dataset_test.cpp )
target_include_directories ( dataset_test PRIVATE ".." )
target_link_libraries( dataset_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )

add_executable ( data_frame_test data_frame_test.cpp )
target_include_directories ( data_frame_test PRIVATE ".." )
target_link_libraries( data_frame_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )

add_executable ( data_frame_view_test data_frame_view_test.cpp )
target_include_directories ( data_frame_view_test PRIVATE ".." )
target_link_libraries( data_frame_view_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )

add_executable ( boost_units_test boost_units_test.cpp )
target_include_directories ( boost_units_test PRIVATE ".." )
target_link_libraries( boost_units_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )
