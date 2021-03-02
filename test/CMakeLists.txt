# ~~~
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~

set(TARGET_NAME "scipp_test_helpers")
set(INC_FILES test_macros.h test_matchers.h test_operations.h test_random.h)

set(SRC_FILES test_matchers.cpp test_random.cpp)

add_library(${TARGET_NAME} STATIC ${INC_FILES} ${SRC_FILES})

target_include_directories(
    ${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
  ${TARGET_NAME} PUBLIC scipp-common scipp-core scipp-variable scipp-dataset scipp-units ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES}
)
