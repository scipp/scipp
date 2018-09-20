cmake_minimum_required(VERSION 3.5)
project(dataset)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" )
include( gsl )
include( pybind11 )
include( GoogleTest )
include( GoogleBenchmark )

find_package (OpenMP 3.1 REQUIRED)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}" )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --save-temps -march=native -fno-omit-frame-pointer")

add_subdirectory ( src )
add_subdirectory ( test )
add_subdirectory ( benchmark )
add_subdirectory ( exports )
