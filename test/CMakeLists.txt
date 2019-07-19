set(INC_FILES test_macros.h test_operations.h)
add_library(scipp_test_helpers INTERFACE)
target_include_directories(scipp_test_helpers INTERFACE .)
