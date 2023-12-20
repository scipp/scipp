# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-dataset")
set(INC_FILES
    ${dataset_INC_FILES}
    include/scipp/dataset/astype.h
    include/scipp/dataset/bin.h
    include/scipp/dataset/bins.h
    include/scipp/dataset/counts.h
    include/scipp/dataset/dataset.h
    include/scipp/dataset/dataset_util.h
    include/scipp/dataset/except.h
    include/scipp/dataset/extract.h
    include/scipp/dataset/groupby.h
    include/scipp/dataset/histogram.h
    include/scipp/dataset/hyperbolic.h
    include/scipp/dataset/math.h
    include/scipp/dataset/mean.h
    include/scipp/dataset/nanmean.h
    include/scipp/dataset/rebin.h
    include/scipp/dataset/shape.h
    include/scipp/dataset/special_values.h
    include/scipp/dataset/sized_dict_forward.h
    include/scipp/dataset/sized_dict.h
    include/scipp/dataset/slice.h
    include/scipp/dataset/sort.h
    include/scipp/dataset/string.h
    include/scipp/dataset/to_unit.h
    include/scipp/dataset/util.h
)

set(SRC_FILES
    arithmetic.cpp
    astype.cpp
    bin.cpp
    bin_detail.cpp
    bins.cpp
    counts.cpp
    data_array.cpp
    dataset.cpp
    except.cpp
    extract.cpp
    groupby.cpp
    histogram.cpp
    mean.cpp
    nanmean.cpp
    operations.cpp
    rebin.cpp
    shape.cpp
    sized_dict.cpp
    slice.cpp
    sort.cpp
    string.cpp
    to_unit.cpp
    util.cpp
    variable_instantiate_bin_elements.cpp
    variable_instantiate_dataset.cpp
    variable_reduction.cpp
    ${dataset_SRC_FILES}
)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
# SCIPP_EXPORT is used in macros defined in variable/
target_compile_definitions(
  ${TARGET_NAME} PRIVATE SCIPP_EXPORT=SCIPP_DATASET_EXPORT
)
target_link_libraries(${TARGET_NAME} PUBLIC scipp-variable)

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME dataset)
add_subdirectory(test)

scipp_install_component(TARGET ${TARGET_NAME} INSTALL_GENERATED)

if(COVERAGE)
  append_coverage_compiler_flags()
endif()
