add_subdirectory ( test )
add_subdirectory ( benchmark )

add_library ( libDataset STATIC dataset.cpp dimensions.cpp unit.cpp variable.cpp )
target_include_directories ( libDataset PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include )
