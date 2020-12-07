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
  configure_file(python/unary_function.cpp.in python/${src})
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
  set(python_SRC_FILES
      ${python_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(variable_includes variable_${category}_includes)
  set(dataset_includes dataset_${category}_includes)
  set(python_binders_fwd python_${category}_binders_fwd)
  set(python_binders python_${category}_binders)
  set(${variable_includes}
      "${${variable_includes}}\n#include \"${variable_inc}\""
      PARENT_SCOPE
  )
  set(${dataset_includes}
      "${${dataset_includes}}\n#include \"${dataset_inc}\""
      PARENT_SCOPE
  )
  set(${python_binders_fwd}
      "${${python_binders_fwd}}\nvoid init_${NAME}(pybind11::module &)ENDL"
      PARENT_SCOPE
  )
  set(${python_binders}
      "${${python_binders}}\n  init_${NAME}(m)ENDL"
      PARENT_SCOPE
  )
endfunction()

function(setup_scipp_category category variable_includes dataset_includes
         python_binders_fwd python_binders
)
  set(include_list ${variable_includes})
  configure_file(
    CMake/generated.h.in
    variable/include/scipp/variable/generated_${category}.h
  )
  set(include_list ${dataset_includes})
  configure_file(
    CMake/generated.h.in dataset/include/scipp/dataset/generated_${category}.h
  )
  string(REPLACE "ENDL" ";" init_list_forward ${python_binders_fwd})
  string(REPLACE "ENDL" ";" init_list ${python_binders})
  configure_file(python/generated.cpp.in python/generated_${category}.cpp)
  set(python_SRC_FILES
      ${python_SRC_FILES} generated_${category}.cpp
      PARENT_SCOPE
  )
endfunction()
