function(scipp_function function_name category)
  message("Generating files for ${function_name}")
  set(NAME ${function_name})
  set(ELEMENT_INCLUDE ${category})
  set(variable_inc scipp/variable/${NAME}.h)
  set(dataset_inc scipp/dataset/${NAME}.h)
  set(src ${NAME}.cpp)
  configure_file(
    variable/include/scipp/variable/unary_function.h.in
    variable/include/${variable_inc}
  )
  configure_file(
    dataset/include/scipp/dataset/unary_function.h.in
    dataset/include/${dataset_inc}
  )
  configure_file(variable/unary_function.cpp.in variable/${src})
  configure_file(dataset/unary_function.cpp.in dataset/${src})
  set(variable_INC_FILES
      ${variable_INC_FILES} "include/${variable_inc}"
      PARENT_SCOPE
  )
  set(dataset_INC_FILES
      ${dataset_INC_FILES} "include/${dataset_inc}"
      PARENT_SCOPE
  )
  set(variable_SRC_FILES
      ${variable_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(dataset_SRC_FILES
      ${dataset_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(category_variable_includes variable_${category}_includes)
  set(category_dataset_includes dataset_${category}_includes)
  set(${category_variable_includes}
      "${${category_variable_includes}}\n#include \"${variable_inc}\""
      PARENT_SCOPE
  )
  set(${category_dataset_includes}
      "${${category_dataset_includes}}\n#include \"${dataset_inc}\""
      PARENT_SCOPE
  )
endfunction()

function(setup_scipp_category variable_includes dataset_includes)
  set(include_list ${variable_includes})
  configure_file(
    CMake/generated.h.in variable/include/scipp/variable/generated_math.h
  )
  set(include_list ${dataset_includes})
  configure_file(
    CMake/generated.h.in dataset/include/scipp/dataset/generated_math.h
  )
endfunction()
