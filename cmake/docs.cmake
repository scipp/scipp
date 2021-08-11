# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
function(add_docs_target name)
  set(oneValueArgs BUILDER)
  set(multiValueArgs DEPENDS)
  cmake_parse_arguments(
    PARSE_ARGV 0 ADD_DOCS_TARGET "" "${oneValueArgs}" "${multiValueArgs}"
  )
  add_custom_target(
    ${name}
    COMMAND
      ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/build_docs.py
      --builder=${ADD_DOCS_TARGET_BUILDER}
      --prefix=${CMAKE_CURRENT_BINARY_DIR}/docs
      --work_dir=${CMAKE_CURRENT_BINARY_DIR}/docs/doctrees
    DEPENDS ${ADD_DOCS_TARGET_DEPENDS}
  )
endfunction()
