function(scipp_function function_name category)
  macro(configure_in_module module name)
    set(inc scipp/${module}/${name}.h)
    set(src ${name}.cpp)
    set(NAME ${function_name})
    configure_file(
      ${module}/include/scipp/${module}/unary_function.h.in
      ${module}/include/${inc}
    )
    configure_file(${module}/unary_function.cpp.in ${module}/${src})
    set(${module}_INC_FILES
        ${${module}_INC_FILES} "include/${inc}"
        PARENT_SCOPE
    )
    set(${module}_SRC_FILES
        ${${module}_SRC_FILES} ${src}
        PARENT_SCOPE
    )
    set(${module}_includes ${module}_${category}_includes)
    set(${${module}_includes}
        "${${${module}_includes}}\n#include \"${inc}\""
        PARENT_SCOPE
    )
  endmacro()
  message("Generating files for ${function_name}")
  set(NAME ${function_name})
  set(ELEMENT_INCLUDE ${category})
  configure_in_module("variable" ${function_name})
  configure_in_module("dataset" ${function_name})
  set(src ${NAME}.cpp)
  configure_file(python/unary_function.cpp.in python/${src})
  set(python_SRC_FILES
      ${python_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(python_binders_fwd python_${category}_binders_fwd)
  set(python_binders python_${category}_binders)
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
