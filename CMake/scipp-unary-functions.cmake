include(scipp-util)

scipp_function(abs math)
scipp_function(exp math)
scipp_function(log math)
scipp_function(log10 math)
scipp_function(reciprocal math)
scipp_function(sqrt math)

setup_scipp_category(math
  ${variable_math_includes} ${dataset_math_includes} ${python_math_binders_fwd}
  ${python_math_binders}
)
