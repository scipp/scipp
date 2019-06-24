# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
pybind11_add_module(scipp
                    SHARED
                    SYSTEM
                    dataset.cpp
                    dimensions.cpp
                    dtype.cpp
                    scippy.cpp
                    units_neutron.cpp
                    variable.cpp
                    variable_view.cpp)
target_link_libraries(scipp LINK_PRIVATE scipp-core)
target_include_directories(scipp SYSTEM PRIVATE "../range-v3/include")

# Set symbol visibility to hidden to reduce binary size, as recommended in pybind11 FAQ.
set_target_properties(scipp PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_sanitizers(scipp)

set(PY_FILES __init__.py table.py plot.py)

install(DIRECTORY "src/scippy" DESTINATION ".")
install(TARGETS scipp DESTINATION "scippy")
