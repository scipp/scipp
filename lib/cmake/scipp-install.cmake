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

# Conda build does not create a dist-info folder like setuptools / scikit-build.
# This function creates a minimal dist-info to satisfy
# https://packaging.python.org/en/latest/specifications/recording-installed-packages/
# There is no need to call this function when building with setuptools.
function(scipp_write_dist_info)
  set(oneValueArgs VERSION)
  cmake_parse_arguments(
    PARSE_ARGV 0 SCIPP_WRITE_DIST_INFO "" "${oneValueArgs}" ""
  )

  set(dist_info_dir ${CMAKE_CURRENT_BINARY_DIR}/dist-info)
  if(DEFINED ENV{SP_DIR}) # conda build
    set(target_dist_info_dir
        "$ENV{SP_DIR}/scipp-${SCIPP_WRITE_DIST_INFO_VERSION}.dist-info"
    )
  else() # fallback, this function should not really be called in this case
    set(target_dist_info_dir
        "${CMAKE_INSTALL_PREFIX}/scipp-${SCIPP_WRITE_DIST_INFO_VERSION}.dist-info"
    )
  endif()
  message(STATUS "Writing dist-info during build to ${dist_info_dir}")
  message(STATUS "dist-info will be installed to ${target_dist_info_dir}")

  set(metadata_file ${dist_info_dir}/METADATA)
  file(
    WRITE ${metadata_file}
    "Metadata-Version: 2.4
Name: ${PROJECT_NAME}
Version: ${SCIPP_WRITE_DIST_INFO_VERSION}
License-Expression: Bsd-3-Clause
License-File: LICENSE
"
  )
  install(FILES "${metadata_file}" DESTINATION "${target_dist_info_dir}")

  install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
          DESTINATION "${target_dist_info_dir}/licenses"
  )
endfunction()
