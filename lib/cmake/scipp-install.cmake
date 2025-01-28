# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
function(scipp_install_component)
  set(options INSTALL_GENERATED NO_EXPORT_HEADER)
  set(oneValueArgs TARGET)
  cmake_parse_arguments(
    PARSE_ARGV 0 SCIPP_INSTALL_COMPONENT "${options}" "${oneValueArgs}" ""
  )
  if(SANITIZERS)
    add_sanitizers(${SCIPP_INSTALL_COMPONENT_TARGET})
  endif()
  if(DYNAMIC_LIB)
    install(
      TARGETS ${SCIPP_INSTALL_COMPONENT_TARGET}
      EXPORT ${EXPORT_NAME}
      RUNTIME_DEPENDENCIES
      PRE_INCLUDE_REGEXES
      ${CONAN_RUNTIME_DEPENDENCIES}
      PRE_EXCLUDE_REGEXES
      ".*"
      # Required for Windows. Other platforms search rpath
      DIRECTORIES
      ${CMAKE_BINARY_DIR}/lib/onetbb/lib
      ${CMAKE_BINARY_DIR}/lib/onetbb/bin
      RUNTIME DESTINATION ${PYTHONDIR}
      ARCHIVE DESTINATION ${ARCHIVEDIR}
      FRAMEWORK DESTINATION ${ARCHIVEDIR}
    )
    if(NOT SKBUILD)
      install(DIRECTORY include/ DESTINATION ${INCLUDEDIR})
      if(${SCIPP_INSTALL_COMPONENT_INSTALL_GENERATED})
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
                DESTINATION ${INCLUDEDIR}
        )
      endif()
      if(NOT ${SCIPP_INSTALL_COMPONENT_NO_EXPORT_HEADER})
        install(
          FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${SCIPP_INSTALL_COMPONENT_TARGET}_export.h
          DESTINATION ${INCLUDEDIR}
        )
      endif()
    endif()
  endif(DYNAMIC_LIB)
endfunction()
