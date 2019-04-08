# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
pybind11_add_module(scippy SHARED dataset.cpp)
target_link_libraries(scippy LINK_PRIVATE scipp-core)
target_include_directories(scippy SYSTEM PRIVATE "../range-v3/include")

set(PY_FILES __init__.py table.py xarray_compat.py plot.py)

install(TARGETS scippy DESTINATION "scippy")
install(FILES ${PY_FILES} DESTINATION "scippy")
install(FILES "compat/mantid.py" DESTINATION "scippy/compat")
