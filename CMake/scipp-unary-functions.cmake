include(scipp-util)

scipp_function(abs math)
scipp_function(exp math)
scipp_function(log math)
scipp_function(log10 math)
scipp_function(reciprocal math)
scipp_function(sqrt math)

configure_file(variable/include/scipp/variable/generated_math.h.in variable/include/scipp/variable/generated_math.h)
