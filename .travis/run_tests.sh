#!/bin/sh

# Build scipp
mkdir -p build
mkdir -p install
cd build
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DPYTHON_EXECUTABLE=/usr/bin/python3 -DCMAKE_INSTALL_PREFIX=../install ..
make -j2 install

python3 -m pip install -r ../scippy/requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
ctest -VV
