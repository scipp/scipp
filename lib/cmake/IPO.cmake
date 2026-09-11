if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION
   OR CMAKE_INTERPROCEDURAL_OPTIMIZATION
)
  include(CheckIPOSupported)
  # Dependencies may enable other languages; Scipp only requires C++ IPO.
  check_ipo_supported(
    RESULT result
    OUTPUT output
    LANGUAGES CXX
  )
  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION
        TRUE
        CACHE BOOL "Link-time optimization: ON/OFF"
    )
  elseif(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(FATAL_ERROR "IPO was requested but is not supported: ${output}")
  else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION
        FALSE
        CACHE BOOL "Link-time optimization: ON/OFF"
    )
    message(WARNING "IPO is not supported: ${output}")
  endif()
endif()
message(STATUS "IPO set to ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")

if(MSVC)
  # CMake IPO does not include LTCG flag, causing the linker to restart
  add_link_options($<$<BOOL:${CMAKE_INTERPROCEDURAL_OPTIMIZATION}>:/LTCG>)
endif()

# See https://github.com/pybind/pybind11/issues/1604
set(INTERPROCEDURAL_OPTIMIZATION_TESTS FALSE)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")
  if(APPLE)
    add_compile_definitions(_APPLE_CLANG_)
  else()
    # For some reason clang6 on ubuntu18 can't link parts with different
    # INTERPROCEDURAL_OPTIMIZATION flag.
    set(INTERPROCEDURAL_OPTIMIZATION_TESTS
        ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}
    )
  endif()
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  # Reduces binary size of, e.g., libscipp-core.so by several MB.
  set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endif()
