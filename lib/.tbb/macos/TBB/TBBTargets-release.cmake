#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "TBB::tbb" for configuration "Release"
set_property(TARGET TBB::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TBB::tbb PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtbb.12.4.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libtbb.12.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS TBB::tbb )
list(APPEND _IMPORT_CHECK_FILES_FOR_TBB::tbb "${_IMPORT_PREFIX}/lib/libtbb.12.4.dylib" )

# Import target "TBB::tbbmalloc" for configuration "Release"
set_property(TARGET TBB::tbbmalloc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TBB::tbbmalloc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtbbmalloc.2.4.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libtbbmalloc.2.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS TBB::tbbmalloc )
list(APPEND _IMPORT_CHECK_FILES_FOR_TBB::tbbmalloc "${_IMPORT_PREFIX}/lib/libtbbmalloc.2.4.dylib" )

# Import target "TBB::tbbmalloc_proxy" for configuration "Release"
set_property(TARGET TBB::tbbmalloc_proxy APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(TBB::tbbmalloc_proxy PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "TBB::tbbmalloc"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtbbmalloc_proxy.2.4.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libtbbmalloc_proxy.2.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS TBB::tbbmalloc_proxy )
list(APPEND _IMPORT_CHECK_FILES_FOR_TBB::tbbmalloc_proxy "${_IMPORT_PREFIX}/lib/libtbbmalloc_proxy.2.4.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
