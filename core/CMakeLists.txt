# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-core")
set(INC_FILES
    include/scipp/core/aligned_allocator.h
    include/scipp/core/dataset.h
    include/scipp/core/dataset_index.h
    include/scipp/core/dimensions.h
    include/scipp/core/except.h
    include/scipp/core/memory_pool.h
    include/scipp/core/tag_util.h
    include/scipp/core/counts.h
    include/scipp/core/slice.h
    include/scipp/core/variable_view.h
    include/scipp/core/variable.h
    include/scipp/core/variable.tcc
    include/scipp/core/vector.h
    include/scipp/core/view_index.h)

set(SRC_FILES
    counts.cpp
    data_array.cpp
    dataset.cpp
    dataset_binary_arithmetic.cpp
    dataset_operations.cpp
    dimensions.cpp
    dtype.cpp
    except.cpp
    rebin.cpp
    slice.cpp
    string.cpp
    trigonometry.cpp
    variable.cpp
    variable_binary_arithmetic.cpp
    variable_instantiate_basic.cpp
    variable_instantiate_dataset.cpp
    variable_operations.cpp
    view_index.cpp)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
                      PUBLIC scipp-common scipp-units Boost::boost)
# Include tcb/span as system header to avoid compiler warnings.
target_include_directories(
  ${TARGET_NAME} SYSTEM
  PUBLIC
    $<INSTALL_INTERFACE:include/scipp/core/span/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/scipp/core/span/include>
  )
target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${TARGET_NAME} SYSTEM
                           PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(test)

if(DYNAMIC_LIB)
  install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif(DYNAMIC_LIB)
