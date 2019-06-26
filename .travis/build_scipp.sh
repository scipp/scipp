#!/bin/sh

# Build scipp
mkdir -p build
mkdir -p install
cd build
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DPYTHON_EXECUTABLE=/usr/bin/python3 -DCMAKE_INSTALL_PREFIX=../install ..
make -j2 install
