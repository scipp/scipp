function(scipp_function function_name category)
  message("Generating files for ${function_name}")
  set(NAME ${function_name})
  set(ELEMENT_INCLUDE ${category})
  set(inc scipp/variable/${NAME}.h)
  set(src ${NAME}.cpp)
  configure_file(
    variable/include/scipp/variable/unary_function.h.in variable/include/${inc}
  )
  configure_file(variable/unary_function.cpp.in variable/${src})
  set(variable_INC_FILES
      ${variable_INC_FILES} "include/${inc}"
      PARENT_SCOPE
  )
  set(variable_SRC_FILES
      ${variable_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(category_includes variable_${category}_includes)
  set(${category_includes}
      "${${category_includes}}\n#include \"${inc}\""
      PARENT_SCOPE
  )
endfunction()
