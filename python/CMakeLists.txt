# ~~~
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
pybind11_add_module(
  _scipp
  SHARED
  SYSTEM
  ${python_SRC_FILES}
  bins.cpp
  choose.cpp
  comparison.cpp
  counts.cpp
  cumulative.cpp
  dataset.cpp
  detail.cpp
  docstring.cpp
  dtype.cpp
  eigen.cpp
  except.cpp
  geometry.cpp
  groupby.cpp
  histogram.cpp
  numpy.cpp
  operations.cpp
  py_object.cpp
  scipp.cpp
  reduction.cpp
  trigonometry.cpp
  unary.cpp
  unit.cpp
  units_neutron.cpp
  variable.cpp
  variable_instantiate_py_object.cpp
  element_array_view.cpp
  shape.cpp
)
pybind11_add_module(
  _scipp_neutron
  SHARED
  SYSTEM
  ${python_SRC_FILES}
  neutron.cpp
  scipp_neutron.cpp
)
target_include_directories(
  _scipp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(
  _scipp_neutron PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(_scipp LINK_PRIVATE scipp-dataset)
target_link_libraries(_scipp_neutron LINK_PRIVATE scipp-neutron)

# SCIPP_EXPORT is used in macros defined in variable/
target_compile_definitions(_scipp PRIVATE SCIPP_EXPORT=)

# Set symbol visibility to hidden to reduce binary size, as recommended in
# pybind11 FAQ.
set_target_properties(_scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(_scipp PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
set_target_properties(_scipp_neutron PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(_scipp_neutron PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

if(PRECOMPILED_HEADERS)
  target_precompile_headers(_scipp PRIVATE pybind11.h)
endif()

add_sanitizers(_scipp)
add_sanitizers(_scipp_neutron)

install(DIRECTORY "src/scipp/" DESTINATION ${PYTHONDIR})
install(TARGETS _scipp DESTINATION ${PYTHONDIR})
install(TARGETS _scipp_neutron DESTINATION ${PYTHONDIR})
