# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
set(INC_FILES
    aligned_allocator.h
    convert.h
    cow_ptr.h
    dataset.h
    dataset_index.h
    dimensions.h
    events.h
    except.h
    function_traits.h
    index.h
    md_zip_view.h
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
    convert.cpp
    counts.cpp
    dataset.cpp
    dimensions.cpp
    events.cpp
    except.cpp
    md_zip_view.cpp
    variable.cpp)

add_library(ScippCore STATIC ${INC_FILES} ${SRC_FILES})
target_link_libraries(ScippCore
                      PUBLIC ScippUnits Boost::boost OpenMP::OpenMP_CXX)
target_include_directories(
  ScippCore
  PUBLIC $<INSTALL_INTERFACE:.> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
  )
target_include_directories(ScippCore SYSTEM
                           PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src
                           PRIVATE "../range-v3/include")
set_target_properties(ScippCore PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
