# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES include/scipp/neutron/convert.h)

set(SRC_FILES convert.cpp
  )

add_library(scipp-neutron STATIC ${INC_FILES} ${SRC_FILES})
target_link_libraries(scipp-neutron PUBLIC scipp-core)
target_include_directories(
  scipp-neutron
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(scipp-neutron SYSTEM
                           PUBLIC ${CMAKE_BINARY_DIR}/Eigen-src
                           PRIVATE "../range-v3/include")
set_target_properties(scipp-neutron PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

#add_subdirectory(test)
