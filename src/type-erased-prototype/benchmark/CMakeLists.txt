set ( BENCH_FILES_SRC
  dataset_benchmark.cpp
)

add_executable ( type_erased_prototype_benchmark ${BENCH_FILES_SRC} )
target_include_directories ( type_erased_prototype_benchmark PRIVATE ".." )
target_link_libraries ( type_erased_prototype_benchmark LINK_PRIVATE ${GBENCH_LIBRARIES} )
