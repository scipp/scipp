set ( PY_FILES
  __init__.py
  table.py
  dataset_plotting.py
  )

install ( FILES ${PY_FILES} DESTINATION "dataset" )
install ( FILES "compat/mantid.py" DESTINATION "dataset/compat" )
