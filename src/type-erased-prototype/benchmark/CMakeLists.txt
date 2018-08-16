add_executable ( dataset_benchmark dataset_benchmark.cpp )
target_link_libraries ( dataset_benchmark LINK_PRIVATE libDataset ${GBENCH_LIBRARIES} )

add_executable ( dataset_view_benchmark dataset_view_benchmark.cpp )
target_link_libraries ( dataset_view_benchmark LINK_PRIVATE libDataset ${GBENCH_LIBRARIES} )

add_executable ( legacy_histogram_benchmark legacy_histogram_benchmark.cpp )
target_link_libraries ( legacy_histogram_benchmark LINK_PRIVATE libDataset ${GBENCH_LIBRARIES} )

add_executable ( multi_index_benchmark multi_index_benchmark.cpp )
target_link_libraries ( multi_index_benchmark LINK_PRIVATE libDataset ${GBENCH_LIBRARIES} )
