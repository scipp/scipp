#!/bin/sh

mkdir -p build
mkdir -p install
cd build
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DPYTHON_EXECUTABLE=/usr/bin/python3 -DCMAKE_INSTALL_PREFIX=../install ..
make -j2 install
./units/test/scipp-units-test
./core/test/scipp-core-test
python3 -m pip install -r ../scippy/requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
cd ../scippy
python3 -m unittest discover test
