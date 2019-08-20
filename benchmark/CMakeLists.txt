# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
add_executable(variable_benchmark EXCLUDE_FROM_ALL variable_benchmark.cpp)
target_link_libraries(variable_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(dataset_benchmark EXCLUDE_FROM_ALL dataset_benchmark.cpp)
target_link_libraries(dataset_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(legacy_histogram_benchmark EXCLUDE_FROM_ALL legacy_histogram_benchmark.cpp)
target_link_libraries(legacy_histogram_benchmark
                      LINK_PRIVATE
                      scipp-core
                      benchmark)

                    add_executable(multi_index_benchmark EXCLUDE_FROM_ALL multi_index_benchmark.cpp)
target_link_libraries(multi_index_benchmark LINK_PRIVATE scipp-core benchmark)

add_executable(variable_view_benchmark EXCLUDE_FROM_ALL variable_view_benchmark.cpp)
target_link_libraries(variable_view_benchmark LINK_PRIVATE scipp-core benchmark)
