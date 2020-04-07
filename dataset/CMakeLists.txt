# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-dataset")
set(INC_FILES
    include/scipp/dataset/dataset.h
    include/scipp/dataset/dataset_index.h
    include/scipp/dataset/except.h
    include/scipp/dataset/counts.h
    include/scipp/dataset/sort.h
)

set(SRC_FILES
    counts.cpp
    data_array.cpp
    dataset.cpp
    dataset_access.cpp
    dataset_binary_arithmetic.cpp
    dataset_operations.cpp
    event.cpp
    except.cpp
    groupby.cpp
    histogram.cpp
    sort.cpp
    string.cpp
    unaligned.cpp
    variable_instantiate_dataset.cpp
    variable_reduction.cpp
)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_link_libraries(
  ${TARGET_NAME} PUBLIC scipp-common scipp-core scipp-units Boost::boost
)
if(TBB_FOUND)
  target_link_libraries(${TARGET_NAME} PUBLIC TBB::tbb)
endif()

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(test)

if(DYNAMIC_LIB)
  install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif(DYNAMIC_LIB)
