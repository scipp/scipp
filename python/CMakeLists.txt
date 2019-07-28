# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
pybind11_add_module(_scipp
                    SHARED
                    SYSTEM
                    dataset.cpp
                    dimensions.cpp
                    dtype.cpp
                    neutron.cpp
                    scipp.cpp
                    sparse_container.cpp
                    units_neutron.cpp
                    variable.cpp
                    variable_view.cpp)
target_link_libraries(_scipp LINK_PRIVATE scipp-core scipp-neutron)
target_include_directories(_scipp SYSTEM PRIVATE "../range-v3/include")

# Set symbol visibility to hidden to reduce binary size, as recommended in pybind11 FAQ.
set_target_properties(_scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(_scipp PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

add_sanitizers(_scipp)

set(PY_FILES __init__.py table.py plot.py tools.py)

install(DIRECTORY "src/scipp" DESTINATION ".")
install(TARGETS _scipp DESTINATION "scipp")
