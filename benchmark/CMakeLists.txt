# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)

add_custom_target(all-benchmarks)

add_executable(groupby_benchmark EXCLUDE_FROM_ALL groupby_benchmark.cpp)
add_dependencies(all-benchmarks groupby_benchmark)
target_link_libraries(groupby_benchmark LINK_PRIVATE scipp-dataset benchmark)

add_executable(slice_benchmark EXCLUDE_FROM_ALL slice_benchmark.cpp)
add_dependencies(all-benchmarks slice_benchmark)
target_link_libraries(slice_benchmark LINK_PRIVATE scipp-dataset benchmark)

add_executable(
  event_filter_benchmark EXCLUDE_FROM_ALL event_filter_benchmark.cpp
)
add_dependencies(all-benchmarks event_filter_benchmark)
target_link_libraries(
  event_filter_benchmark LINK_PRIVATE benchmark scipp-dataset
  scipp_test_helpers
)

add_executable(histogram_benchmark EXCLUDE_FROM_ALL histogram_benchmark.cpp)
add_dependencies(all-benchmarks histogram_benchmark)
target_link_libraries(
  histogram_benchmark LINK_PRIVATE benchmark scipp-dataset scipp_test_helpers
)

add_executable(
  events_histogram_op_benchmark EXCLUDE_FROM_ALL
  events_histogram_op_benchmark.cpp
)
add_dependencies(all-benchmarks events_histogram_op_benchmark)
target_link_libraries(
  events_histogram_op_benchmark LINK_PRIVATE benchmark scipp-dataset
  scipp_test_helpers
)

add_executable(
  neutron_convert_benchmark EXCLUDE_FROM_ALL neutron_convert_benchmark.cpp
)
add_dependencies(all-benchmarks neutron_convert_benchmark)
target_link_libraries(
  neutron_convert_benchmark LINK_PRIVATE benchmark scipp-dataset scipp-neutron
  scipp_test_helpers
)

add_executable(transform_benchmark EXCLUDE_FROM_ALL transform_benchmark.cpp)
add_dependencies(all-benchmarks transform_benchmark)
target_link_libraries(transform_benchmark LINK_PRIVATE scipp-variable benchmark)

add_executable(variable_benchmark EXCLUDE_FROM_ALL variable_benchmark.cpp)
add_dependencies(all-benchmarks variable_benchmark)
target_link_libraries(variable_benchmark LINK_PRIVATE scipp-variable benchmark)

add_executable(dataset_benchmark EXCLUDE_FROM_ALL dataset_benchmark.cpp)
add_dependencies(all-benchmarks dataset_benchmark)
target_link_libraries(dataset_benchmark LINK_PRIVATE scipp-dataset benchmark)

add_executable(
  dataset_operations_benchmark EXCLUDE_FROM_ALL
  dataset_operations_benchmark.cpp
)
add_dependencies(all-benchmarks dataset_operations_benchmark)
target_link_libraries(
  dataset_operations_benchmark LINK_PRIVATE scipp-dataset benchmark
)

add_executable(
  legacy_histogram_benchmark EXCLUDE_FROM_ALL legacy_histogram_benchmark.cpp
)
# add_dependencies(all-benchmarks legacy_histogram_benchmark)
target_link_libraries(
  legacy_histogram_benchmark LINK_PRIVATE scipp-core benchmark
)

add_executable(multi_index_benchmark EXCLUDE_FROM_ALL multi_index_benchmark.cpp)
# add_dependencies(all-benchmarks multi_index_benchmark)
target_link_libraries(multi_index_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(
  element_array_view_benchmark EXCLUDE_FROM_ALL
  element_array_view_benchmark.cpp
)
# add_dependencies(all-benchmarks element_array_view_benchmark)
target_link_libraries(
  element_array_view_benchmark LINK_PRIVATE scipp-core benchmark
)
