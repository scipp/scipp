# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)

add_custom_target(all-benchmarks)

add_executable(transform_benchmark EXCLUDE_FROM_ALL transform_benchmark.cpp)
add_dependencies(all-benchmarks transform_benchmark)
target_link_libraries(transform_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(variable_benchmark EXCLUDE_FROM_ALL variable_benchmark.cpp)
add_dependencies(all-benchmarks variable_benchmark)
target_link_libraries(variable_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(dataset_benchmark EXCLUDE_FROM_ALL dataset_benchmark.cpp)
add_dependencies(all-benchmarks dataset_benchmark)
target_link_libraries(dataset_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(legacy_histogram_benchmark EXCLUDE_FROM_ALL legacy_histogram_benchmark.cpp)
# add_dependencies(all-benchmarks legacy_histogram_benchmark)
target_link_libraries(legacy_histogram_benchmark
                      LINK_PRIVATE
                      scipp-core
                      benchmark)

add_executable(multi_index_benchmark EXCLUDE_FROM_ALL multi_index_benchmark.cpp)
# add_dependencies(all-benchmarks multi_index_benchmark)
target_link_libraries(multi_index_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(variable_view_benchmark EXCLUDE_FROM_ALL variable_view_benchmark.cpp)
# add_dependencies(all-benchmarks variable_view_benchmark)
target_link_libraries(variable_view_benchmark LINK_PRIVATE scipp-core benchmark)
