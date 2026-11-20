# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~

# Keep Release optimizations for hot bindings and enable free-threaded Python.
nanobind_add_module(
  _scipp
  NOMINSIZE
  NB_SUPPRESS_WARNINGS
  # Keep Scipp's type registry and warning settings private to this module.
  NB_DOMAIN
  scipp
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
  numpy_c_api.cpp
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

if(CMAKE_CXX_CPPCHECK)
  # CMake omits system includes when running cppcheck. Skip nanobind's member
  # declarations so the checker still analyzes Scipp's caster and its callers.
  set_property(
    TARGET _scipp
    APPEND
    PROPERTY CXX_CPPCHECK "-DNB_TYPE_CASTER(T,descr)="
  )
  foreach(runtime IN ITEMS nanobind-static nanobind-static-ft)
    if(TARGET ${runtime})
      set_property(TARGET ${runtime} PROPERTY CXX_CPPCHECK "")
    endif()
  endforeach()
endif()

target_include_directories(
  _scipp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(_scipp LINK_PRIVATE scipp-dataset Python::NumPy)

# SCIPP_EXPORT is used in macros defined in variable/
target_compile_definitions(
  _scipp PRIVATE SCIPP_EXPORT= NPY_NO_DEPRECATED_API=NPY_2_0_API_VERSION
)

set_target_properties(
  _scipp
  PROPERTIES INSTALL_RPATH
             "${CMAKE_INSTALL_RPATH}/${RELATIVE_LIB}/${CMAKE_INSTALL_LIBDIR}"
)

# Only scipp.cpp needs the version; keeping it off other TUs means a new git
# commit doesn't change every compile command (full rebuild + ccache misses).
set_source_files_properties(
  scipp.cpp PROPERTIES COMPILE_DEFINITIONS
                       "SCIPP_VERSION=\"${SCIPP_VERSION_PEP440}\""
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
