# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES
    include/scipp/core/aligned_allocator.h
    include/scipp/core/dataset.h
    include/scipp/core/dataset_index.h
    include/scipp/core/dimensions.h
    include/scipp/core/events.h
    include/scipp/core/except.h
    include/scipp/core/index.h
    include/scipp/core/memory_pool.h
    include/scipp/core/span.h
    include/scipp/core/tag_util.h
    include/scipp/core/counts.h
    include/scipp/core/traits.h
    include/scipp/core/variable_view.h
    include/scipp/core/variable.h
    include/scipp/core/vector.h
    include/scipp/core/view_index.h)

set(SRC_FILES
    counts.cpp
    dataset.cpp
    dimensions.cpp
    #events.cpp
    except.cpp
    variable.cpp
    variable_binary_ops.cpp
    view_index.cpp)

add_library(scipp-core SHARED ${INC_FILES} ${SRC_FILES})
target_link_libraries(scipp-core
                      PUBLIC scipp-units Boost::boost OpenMP::OpenMP_CXX)
# Include tcb/span as system header to avoid compiler warnings.
target_include_directories(
  scipp-core SYSTEM
  PUBLIC
    $<INSTALL_INTERFACE:include/scipp/core/span/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/scipp/core/span/include>
  )
target_include_directories(
  scipp-core
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(scipp-core SYSTEM
                           PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src
                           PRIVATE "../range-v3/include")
set_target_properties(scipp-core PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
install(TARGETS scipp-core LIBRARY DESTINATION lib)

add_subdirectory(test)
