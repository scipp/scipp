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
find_package (Boost 1.59 REQUIRED)

include( gsl )
include( pybind11 )
include( GoogleTest )
include( GoogleBenchmark )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --save-temps -march=native -fno-omit-frame-pointer")

add_subdirectory ( src )
add_subdirectory ( test )
add_subdirectory ( benchmark )
add_subdirectory ( exports )
add_subdirectory ( python )
