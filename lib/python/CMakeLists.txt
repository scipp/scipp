# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~

# Use Python_add_library since pybind11_add_module causes linking to libpython,
# which should be avoided on Linux and OSX.
python_add_library(
  _scipp
  MODULE
  bind_units.cpp
  bins.cpp
  comparison.cpp
  counts.cpp
  variable_creation.cpp
  cumulative.cpp
  dataset.cpp
  docstring.cpp
  dtype.cpp
  except.cpp
  geometry.cpp
  groupby.cpp
  histogram.cpp
  numpy.cpp
  operations.cpp
  py_object.cpp
  scipp.cpp
  transform.cpp
  trigonometry.cpp
  unary.cpp
  unit.cpp
  variable.cpp
  variable_init.cpp
  variable_instantiate_py_object.cpp
  element_array_view.cpp
  shape.cpp
  slice_utils.cpp
  ${python_SRC_FILES}
)

target_include_directories(
  _scipp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(_scipp LINK_PRIVATE scipp-dataset pybind11::headers)
target_compile_definitions(_scipp PUBLIC PYBIND11_DETAILED_ERROR_MESSAGES)

# SCIPP_EXPORT is used in macros defined in variable/
target_compile_definitions(_scipp PRIVATE SCIPP_EXPORT=)

# Set symbol visibility to hidden to reduce binary size, as recommended in
# pybind11 FAQ.
set_target_properties(_scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(
  _scipp
  PROPERTIES INSTALL_RPATH
             "${CMAKE_INSTALL_RPATH}/${RELATIVE_LIB}/${CMAKE_INSTALL_LIBDIR}"
)

if(PRECOMPILED_HEADERS)
  target_precompile_headers(_scipp PRIVATE pybind11.h)
endif()

if(SANITIZERS)
  add_sanitizers(_scipp)
endif()

install(TARGETS _scipp DESTINATION ${PYTHONDIR})

# On Windows, explicitly install TBB DLL next to _scipp.pyd This is needed
# because TBB's install puts the DLL in bin/, but _scipp.pyd needs it in the
# same directory to find it at runtime.
if(WIN32 AND TARGET TBB::tbb)
  install(FILES $<TARGET_FILE:TBB::tbb> DESTINATION ${PYTHONDIR})
endif()

if(COVERAGE)
  append_coverage_compiler_flags()
endif()
