cmake_minimum_required(VERSION 3.9)
project(dataset)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" )

find_package (OpenMP 3.1 REQUIRED)
find_package (Boost 1.67 REQUIRED)

include( Eigen )
include( gsl )
include( pybind11 )
include( GoogleTest )
include( GoogleBenchmark )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -fno-omit-frame-pointer")

# See https://github.com/pybind/pybind11/issues/1604
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")
  if(APPLE)
    add_compile_definitions(_APPLE_CLANG_)
  endif()
endif()

option(WITH_ASAN "Enable address sanitizer" OFF)
if(WITH_ASAN)
  message(STATUS "Enabling address sanitizer")
  add_compile_options(-fno-omit-frame-pointer -fno-common -fsanitize=address)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address -lasan -static-libasan" )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -lasan -static-libasan" )
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -lasan -static-libasan" )
endif()

add_compile_options ( -Wall -Wextra -Winit-self -Wpointer-arith
                      -Wcast-qual -Wcast-align -Wno-self-assign-overloaded)
add_compile_options ( $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>
)
# This is currently causing to many warnings, reenable when appropriate.
#add_compile_options ( -Wconversion )

add_subdirectory ( src )
add_subdirectory ( test )
add_subdirectory ( benchmark )
add_subdirectory ( exports )
add_subdirectory ( python )
