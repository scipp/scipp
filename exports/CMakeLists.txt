pybind11_add_module ( dataset SHARED dataset.cpp )
target_link_libraries ( dataset
  LINK_PRIVATE
  Dataset
  )
install ( TARGETS dataset DESTINATION "dataset" )
