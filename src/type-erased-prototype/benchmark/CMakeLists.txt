add_executable ( dataset_benchmark dataset_benchmark.cpp )
target_include_directories ( dataset_benchmark PRIVATE ".." )
target_link_libraries ( dataset_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )

add_executable ( dataset_iterator_benchmark dataset_iterator_benchmark.cpp )
target_include_directories ( dataset_iterator_benchmark PRIVATE ".." )
target_link_libraries ( dataset_iterator_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )
