# ~~~
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-neutron")
set(INC_FILES include/scipp/neutron/convert.h include/scipp/neutron/beamline.h
              include/scipp/neutron/diffraction/convert_with_calibration.h
)

set(SRC_FILES beamline.cpp convert.cpp diffraction/convert_with_calibration.cpp)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PUBLIC scipp-dataset)
target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_include_directories(
  ${TARGET_NAME} SYSTEM PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)

if(DYNAMIC_LIB)
  install(TARGETS ${TARGET_NAME})
  install(DIRECTORY include/ DESTINATION ${INCLUDEDIR})
endif(DYNAMIC_LIB)
