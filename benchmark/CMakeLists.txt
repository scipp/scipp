# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable ( variable_benchmark variable_benchmark.cpp )
target_link_libraries ( variable_benchmark LINK_PRIVATE Dataset benchmark )

add_executable ( dataset_benchmark dataset_benchmark.cpp )
target_link_libraries ( dataset_benchmark LINK_PRIVATE Dataset benchmark )

add_executable ( md_zip_view_benchmark md_zip_view_benchmark.cpp )
target_link_libraries ( md_zip_view_benchmark LINK_PRIVATE Dataset benchmark )

add_executable ( legacy_histogram_benchmark legacy_histogram_benchmark.cpp )
target_link_libraries ( legacy_histogram_benchmark LINK_PRIVATE Dataset benchmark )

add_executable ( multi_index_benchmark multi_index_benchmark.cpp )
target_link_libraries ( multi_index_benchmark LINK_PRIVATE Dataset benchmark )

add_executable ( event_list_proxy_benchmark event_list_proxy_benchmark.cpp )
target_link_libraries ( event_list_proxy_benchmark LINK_PRIVATE Dataset benchmark )
