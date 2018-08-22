add_subdirectory ( test )
add_subdirectory ( benchmark )

add_library ( Dataset STATIC dataset.cpp dataset_view.cpp dimensions.cpp unit.cpp variable.cpp )
target_include_directories ( Dataset PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include )
