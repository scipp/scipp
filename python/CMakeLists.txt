set ( PY_FILES
  __init__.py
  xarray_compat.py
  )

install ( FILES ${PY_FILES} DESTINATION "dataset" )
install ( FILES "compat/mantid.py" DESTINATION "dataset/compat" )
