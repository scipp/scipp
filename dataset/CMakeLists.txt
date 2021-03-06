# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-dataset")
set(INC_FILES
    ${dataset_INC_FILES}
    include/scipp/dataset/astype.h
    include/scipp/dataset/bin.h
    include/scipp/dataset/bins.h
    include/scipp/dataset/choose.h
    include/scipp/dataset/counts.h
    include/scipp/dataset/dataset.h
    include/scipp/dataset/dataset_util.h
    include/scipp/dataset/except.h
    include/scipp/dataset/groupby.h
    include/scipp/dataset/histogram.h
    include/scipp/dataset/map_view_forward.h
    include/scipp/dataset/map_view.h
    include/scipp/dataset/math.h
    include/scipp/dataset/rebin.h
    include/scipp/dataset/reduction.h
    include/scipp/dataset/special_values.h
    include/scipp/dataset/util.h
    include/scipp/dataset/slice.h
    include/scipp/dataset/shape.h
    include/scipp/dataset/sort.h
    include/scipp/dataset/string.h
    include/scipp/dataset/to_unit.h
)

set(SRC_FILES
    ${dataset_SRC_FILES}
    arithmetic.cpp
    astype.cpp
    bin.cpp
    bins.cpp
    counts.cpp
    data_array.cpp
    dataset.cpp
    except.cpp
    groupby.cpp
    histogram.cpp
    map_view.cpp
    operations.cpp
    rebin.cpp
    reduction.cpp
    util.cpp
    shape.cpp
    slice.cpp
    sort.cpp
    string.cpp
    to_unit.cpp
    variable_instantiate_bin_elements.cpp
    variable_instantiate_dataset.cpp
    variable_reduction.cpp
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
target_link_libraries(${TARGET_NAME} PUBLIC scipp-variable Boost::boost)
if(TBB_FOUND AND NOT DISABLE_MULTI_THREADING)
  target_link_libraries(${TARGET_NAME} PUBLIC TBB::tbb)
endif()

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
