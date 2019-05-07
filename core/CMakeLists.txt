# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES
    aligned_allocator.h
    convert.h
    dataset.h
    dataset_index.h
    dimensions.h
    events.h
    except.h
    function_traits.h
    index.h
    #md_zip_view.h
    memory_pool.h
    multi_index.h
    span.h
    tags.h
    tag_util.h
    traits.h
    value_with_delta.h
    variable_view.h
    variable.h
    vector.h)

set(SRC_FILES
    #convert.cpp
    #counts.cpp
    dataset.cpp
    dimensions.cpp
    #events.cpp
    except.cpp
    #md_zip_view.cpp
    variable.cpp)

add_library(scipp-core STATIC ${INC_FILES} ${SRC_FILES})
target_link_libraries(scipp-core
                      PUBLIC scipp-units Boost::boost OpenMP::OpenMP_CXX)
target_include_directories(
  scipp-core
  PUBLIC $<INSTALL_INTERFACE:.> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
  )
target_include_directories(scipp-core SYSTEM
                           PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src
                           PRIVATE "../range-v3/include")
set_target_properties(scipp-core PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
