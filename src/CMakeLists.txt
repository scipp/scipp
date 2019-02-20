# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
set(INC_FILES
  aligned_allocator.h
  convert.h 
  cow_ptr.h
  dataset.h 
  dataset_index.h
  dimensions.h 
  except.h
  function_traits.h
  md_zip_view.h 
  memory_pool.h
  multi_index.h
  tags.h
  tag_util.h
  traits.h
  unit.h 
  value_with_delta.h
  variable_view.h
  variable.h 
  vector.h
)

set(SRC_FILES
  convert.cpp 
  dataset.cpp 
  dimensions.cpp
  except.cpp
  md_zip_view.cpp
  unit.cpp
  variable.cpp
)

add_library ( Dataset STATIC  ${INC_FILES} ${SRC_FILES})
target_link_libraries ( Dataset PUBLIC Boost::boost OpenMP::OpenMP_CXX )
target_include_directories ( Dataset SYSTEM PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include ${CMAKE_BINARY_DIR}/Eigen-src PRIVATE "../range-v3/include" )
set_target_properties ( Dataset PROPERTIES POSITION_INDEPENDENT_CODE TRUE )
