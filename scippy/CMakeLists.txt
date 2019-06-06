# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
pybind11_add_module(scippy
                    SHARED
                    dimensions.cpp
                    scippy.cpp
                    units-neutron.cpp
                    variable.cpp)
target_link_libraries(scippy LINK_PRIVATE scipp-core)
target_include_directories(scippy SYSTEM PRIVATE "../range-v3/include")

# Set symbol visibility to hidden to reduce binary size, as recommended in pybind11 FAQ.
set_target_properties(scippy PROPERTIES CXX_VISIBILITY_PRESET hidden)

set(PY_FILES __init__.py table.py plot.py)

install(TARGETS scippy DESTINATION "scippy")
install(FILES ${PY_FILES} DESTINATION "scippy")
install(FILES "compat/mantid.py" DESTINATION "scippy/compat")
