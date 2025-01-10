# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
add_custom_target(all-benchmarks)

add_executable(groupby_benchmark groupby_benchmark.cpp)
add_dependencies(all-benchmarks groupby_benchmark)
target_link_libraries(
  groupby_benchmark LINK_PRIVATE scipp-dataset benchmark::benchmark_main
)

add_executable(bin_benchmark bin_benchmark.cpp)
add_dependencies(all-benchmarks bin_benchmark)
target_link_libraries(
  bin_benchmark LINK_PRIVATE scipp-dataset benchmark::benchmark_main
)

add_executable(buckets_benchmark buckets_benchmark.cpp)
add_dependencies(all-benchmarks buckets_benchmark)
target_link_libraries(
  buckets_benchmark LINK_PRIVATE scipp-dataset benchmark::benchmark_main
)

add_executable(slice_benchmark slice_benchmark.cpp)
add_dependencies(all-benchmarks slice_benchmark)
target_link_libraries(
  slice_benchmark LINK_PRIVATE scipp-dataset benchmark::benchmark_main
)

add_executable(histogram_benchmark histogram_benchmark.cpp)
add_dependencies(all-benchmarks histogram_benchmark)
target_link_libraries(
  histogram_benchmark LINK_PRIVATE benchmark::benchmark_main scipp-dataset
  scipp_test_helpers
)

add_executable(events_histogram_op_benchmark events_histogram_op_benchmark.cpp)
add_dependencies(all-benchmarks events_histogram_op_benchmark)
target_link_libraries(
  events_histogram_op_benchmark LINK_PRIVATE benchmark::benchmark_main
  scipp-dataset scipp_test_helpers
)

add_executable(transform_benchmark transform_benchmark.cpp)
add_dependencies(all-benchmarks transform_benchmark)
target_link_libraries(
  transform_benchmark LINK_PRIVATE scipp-variable benchmark::benchmark_main
)

add_executable(accumulate_benchmark accumulate_benchmark.cpp)
add_dependencies(all-benchmarks accumulate_benchmark)
target_link_libraries(
  accumulate_benchmark LINK_PRIVATE scipp-variable benchmark::benchmark_main
)

add_executable(variable_benchmark variable_benchmark.cpp)
add_dependencies(all-benchmarks variable_benchmark)
target_link_libraries(
  variable_benchmark LINK_PRIVATE scipp-variable benchmark::benchmark_main
)

add_executable(dataset_benchmark dataset_benchmark.cpp)
add_dependencies(all-benchmarks dataset_benchmark)
target_link_libraries(
  dataset_benchmark LINK_PRIVATE scipp-dataset benchmark::benchmark_main
)

add_executable(dataset_operations_benchmark dataset_operations_benchmark.cpp)
add_dependencies(all-benchmarks dataset_operations_benchmark)
target_link_libraries(
  dataset_operations_benchmark LINK_PRIVATE scipp-dataset
  benchmark::benchmark_main
)

add_executable(
  legacy_histogram_benchmark EXCLUDE_FROM_ALL legacy_histogram_benchmark.cpp
)
# add_dependencies(all-benchmarks legacy_histogram_benchmark)
target_link_libraries(
  legacy_histogram_benchmark LINK_PRIVATE scipp-core benchmark::benchmark_main
)

add_executable(multi_index_benchmark multi_index_benchmark.cpp)
# add_dependencies(all-benchmarks multi_index_benchmark)
target_link_libraries(
  multi_index_benchmark LINK_PRIVATE scipp-core benchmark::benchmark_main
)

add_executable(element_array_view_benchmark element_array_view_benchmark.cpp)
add_dependencies(all-benchmarks element_array_view_benchmark)
target_link_libraries(
  element_array_view_benchmark LINK_PRIVATE scipp-core
  benchmark::benchmark_main
)
