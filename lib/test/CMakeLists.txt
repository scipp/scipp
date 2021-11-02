set(INC_FILES random.h test_macros.h test_nans.h test_operations.h
              test_print_variable.h test_util.h
)
add_library(scipp_test_helpers INTERFACE)
target_include_directories(scipp_test_helpers INTERFACE .)
