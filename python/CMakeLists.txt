# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
pybind11_add_module(
  _scipp
  SHARED
  SYSTEM
  choose.cpp
  comparison.cpp
  counts.cpp
  dataset.cpp
  detail.cpp
  docstring.cpp
  dtype.cpp
  eigen.cpp
  geometry.cpp
  groupby.cpp
  histogram.cpp
  neutron.cpp
  operations.cpp
  py_object.cpp
  scipp.cpp
  event_list.cpp
  reduction.cpp
  trigonometry.cpp
  unary.cpp
  units_neutron.cpp
  variable.cpp
  variable_instantiate_py_object.cpp
  element_array_view.cpp
)
target_link_libraries(_scipp LINK_PRIVATE scipp-dataset scipp-neutron)

# Set symbol visibility to hidden to reduce binary size, as recommended in
# pybind11 FAQ.
set_target_properties(_scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(_scipp PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

add_sanitizers(_scipp)

set(PY_FILES
    scipp/src/__init__.py
    scipp/src/extend_units.py
    scipp/src/object_list.py
    scipp/src/show.py
    scipp/src/table.py
    scipp/src/compat/mantid.py
    scipp/src/neutron/__init__.py
    scipp/src/neutron/exceptions.py
    scipp/src/neutron/load.py
    scipp/src/neutron/diffraction/__init__.py
    scipp/src/neutron/diffraction/load.py
    scipp/src/plot/__init__.py
    scipp/src/plot/config.py
    scipp/src/plot/dispatch.py
    scipp/src/plot/plot_1d.py
    scipp/src/plot/plot_2d.py
    scipp/src/plot/plot_3d.py
    scipp/src/plot/plot_collapse.py
    scipp/src/plot/plot_matplotlib.py
    scipp/src/plot/plot.py
    scipp/src/plot/tools.py
)

install(DIRECTORY "src/scipp" DESTINATION ".")
install(TARGETS _scipp DESTINATION "scipp")
