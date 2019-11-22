#!/bin/bash

set -xe

# Build scipp
mkdir -p build
mkdir -p install
cd build
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install ..
make -j2 install all-tests all-benchmarks

# C++ tests
./common/test/scipp-common-test
./units/test/scipp-units-test
./core/test/scipp-core-test
./neutron/test/scipp-neutron-test

# Python tests
python3 -m pip install --user -r ../python/requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
cd ../python
python3 -m pytest
