function(scipp_function template category function_name)
  set(options SKIP_VARIABLE NO_OUT)
  set(oneValueArgs OP)
  cmake_parse_arguments(
    PARSE_ARGV 3 SCIPP_FUNCTION "${options}" "${oneValueArgs}" ""
  )

  message("Generating files for ${function_name}")
  set(NAME ${function_name})
  set(ELEMENT_INCLUDE ${category})
  if(SCIPP_FUNCTION_NO_OUT)
    set(GENERATE_OUT "false")
  else()
    set(GENERATE_OUT "true")
  endif()
  if(DEFINED SCIPP_FUNCTION_OP)
    set(OPNAME ${SCIPP_FUNCTION_OP})
  else()
    set(OPNAME ${NAME})
  endif()
  set(src ${OPNAME}.cpp)

  macro(configure_in_module module name)
    set(inc scipp/${module}/${name}.h)
    configure_file(
      templates/${module}_${template}.h.in ${module}/include/${inc}
    )
    configure_file(templates/${module}_${template}.cpp.in ${module}/${src})
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

  if(NOT SCIPP_FUNCTION_SKIP_VARIABLE)
    configure_in_module("variable" ${OPNAME})
  endif()
  configure_in_module("dataset" ${OPNAME})
  configure_file(templates/python_${template}.cpp.in python/${src})
  set(python_SRC_FILES
      ${python_SRC_FILES} ${src}
      PARENT_SCOPE
  )
  set(python_binders_fwd python_${category}_binders_fwd)
  set(python_binders python_${category}_binders)
  set(${python_binders_fwd}
      "${${python_binders_fwd}}\nvoid init_${OPNAME}(pybind11::module &)ENDL"
      PARENT_SCOPE
  )
  set(${python_binders}
      "${${python_binders}}\n  init_${OPNAME}(m)ENDL"
      PARENT_SCOPE
  )
endfunction()

macro(scipp_unary)
  scipp_function("unary" ${ARGV})
endmacro()

macro(scipp_binary category function_name)
  scipp_function("binary" ${category} ${function_name})
endmacro()

function(setup_scipp_category category)
  set(include_list ${variable_${category}_includes})
  configure_file(
    CMake/generated.h.in
    variable/include/scipp/variable/generated_${category}.h
  )
  set(include_list ${dataset_${category}_includes})
  configure_file(
    CMake/generated.h.in dataset/include/scipp/dataset/generated_${category}.h
  )
  string(REPLACE "ENDL" ";" init_list_forward ${python_${category}_binders_fwd})
  string(REPLACE "ENDL" ";" init_list ${python_${category}_binders})
  configure_file(python/generated.cpp.in python/generated_${category}.cpp)
  set(python_SRC_FILES
      ${python_SRC_FILES} generated_${category}.cpp
      PARENT_SCOPE
  )
endfunction()
