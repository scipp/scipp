pybind11_add_module(scippy SHARED dataset.cpp)
target_link_libraries(scippy LINK_PRIVATE Dataset)

set(PY_FILES __init__.py table.py xarray_compat.py plot.py)

install(TARGETS scippy DESTINATION "scippy")
install(FILES ${PY_FILES} DESTINATION "scippy")
install(FILES "compat/mantid.py" DESTINATION "scippy/compat")
