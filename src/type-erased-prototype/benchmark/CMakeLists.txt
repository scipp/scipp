add_executable ( dataset_benchmark dataset_benchmark.cpp ../dimensions.cpp )
target_include_directories ( dataset_benchmark PRIVATE ".." )
target_link_libraries ( dataset_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )

add_executable ( dataset_view_benchmark dataset_view_benchmark.cpp ../dimensions.cpp )
target_include_directories ( dataset_view_benchmark PRIVATE ".." )
target_link_libraries ( dataset_view_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )

add_executable ( legacy_histogram_benchmark legacy_histogram_benchmark.cpp ../dimensions.cpp )
target_include_directories ( legacy_histogram_benchmark PRIVATE ".." )
target_link_libraries ( legacy_histogram_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )
