# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-variable")
set(INC_FILES
    include/scipp/variable/apply.h
    include/scipp/variable/event.h
    include/scipp/variable/except.h
    include/scipp/variable/indexed_slice_view.h
    include/scipp/variable/string.h
    include/scipp/variable/subspan_view.h
    include/scipp/variable/transform.h
    include/scipp/variable/transform_subspan.h
    include/scipp/variable/variable_binary_arithmetic.h
    include/scipp/variable/variable_concept.h
    include/scipp/variable/variable.h
    include/scipp/variable/variable.tcc
    include/scipp/variable/variable_comparison.h
    include/scipp/variable/variable_keyword_arg_constructor.h
    include/scipp/variable/variable_logical.h
    include/scipp/variable/variable_misc_operations.h
    include/scipp/variable/variable_operations.h
    include/scipp/variable/variable_reduction.h
    include/scipp/variable/variable_trigonometry.h
    include/scipp/variable/visit.h
)

set(SRC_FILES
    event.cpp
    except.cpp
    rebin.cpp
    string.cpp
    subspan_view.cpp
    variable_binary_arithmetic.cpp
    variable_concept.cpp
    variable.cpp
    variable_comparison.cpp
    variable_inplace_arithmetic.cpp
    variable_instantiate_basic.cpp
    variable_instantiate_view_elements.cpp
    variable_logical.cpp
    variable_operations.cpp
    variable_reduction.cpp
    variable_trigonometry.cpp
    variable_type_conversion.cpp
)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PUBLIC scipp-core Boost::boost)
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
