# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
find_package(Python 3.8 REQUIRED COMPONENTS Interpreter)
function(add_docs_target name)
  set(oneValueArgs BUILDER)
  set(multiValueArgs DEPENDS)
  cmake_parse_arguments(
    PARSE_ARGV 0 ADD_DOCS_TARGET "" "${oneValueArgs}" "${multiValueArgs}"
  )
  add_custom_target(
    ${name}
    COMMAND
      ${Python_EXECUTABLE} -m sphinx -j2 -v -b ${ADD_DOCS_TARGET_BUILDER} -d
      ${CMAKE_BINARY_DIR}/.doctrees ${CMAKE_SOURCE_DIR}/docs
      ${CMAKE_BINARY_DIR}/html
    DEPENDS ${ADD_DOCS_TARGET_DEPENDS}
  )
endfunction()
