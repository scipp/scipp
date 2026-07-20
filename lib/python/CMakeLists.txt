# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~

# nanobind_add_module avoids linking to libpython (it uses Python::Module),
# which should be avoided on Linux and OSX. NB_STATIC compiles the small
# nanobind runtime into the module instead of a shared libnanobind.
# FREE_THREADED marks the module as safe for free-threaded CPython; it is a
# no-op for GIL-enabled interpreters.
nanobind_add_module(
  _scipp
  NB_STATIC
  FREE_THREADED
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

# nanobind_add_module(... NB_STATIC ...) compiles nanobind's own runtime sources
# (e.g. nb_internals.cpp) into a "nanobind-static" target in this directory
# scope, so it inherits our strict warning flags (-Wall, -Wold-style-cast, etc.)
# set in the parent CMakeLists.txt. That vendor code is not under our control,
# so disable warnings for it explicitly.
if(TARGET nanobind-static)
  target_compile_options(
    nanobind-static PRIVATE $<$<NOT:$<PLATFORM_ID:Windows>>:-w>
  )
endif()

target_include_directories(
  _scipp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(_scipp LINK_PRIVATE scipp-dataset)

# SCIPP_EXPORT is used in macros defined in variable/
target_compile_definitions(_scipp PRIVATE SCIPP_EXPORT=)

# Set symbol visibility to hidden to reduce binary size, as recommended in
# nanobind FAQ.
set_target_properties(_scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(
  _scipp
  PROPERTIES INSTALL_RPATH
             "${CMAKE_INSTALL_RPATH}/${RELATIVE_LIB}/${CMAKE_INSTALL_LIBDIR}"
)

if(PRECOMPILED_HEADERS)
  target_precompile_headers(_scipp PRIVATE nanobind.h)
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
