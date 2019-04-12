# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
pybind11_add_module(scippy SHARED dataset.cpp)
target_link_libraries(scippy LINK_PRIVATE scipp-core)
target_include_directories(scippy SYSTEM PRIVATE "../range-v3/include")

set(PY_FILES __init__.py table.py plot.py)

install(TARGETS scippy DESTINATION "scippy")
install(FILES ${PY_FILES} DESTINATION "scippy")
install(FILES "compat/mantid.py" DESTINATION "scippy/compat")
file(GLOB test_files
    "test/*.py"
)
foreach(test_file ${test_files})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test("scippy/${test_name}" "python3" ${test_file})
endforeach()
