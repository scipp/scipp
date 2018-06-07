add_executable ( type_erased_prototype_test dataset_test.cpp dataset_iterator_test.cpp histogram_test.cpp data_array_test.cpp ../dimensions.cpp dimensions_test.cpp )
target_include_directories ( type_erased_prototype_test PRIVATE ".." )
target_link_libraries( type_erased_prototype_test
  LINK_PRIVATE
  gtest_main
  ${GTEST_LIBRARIES} )
