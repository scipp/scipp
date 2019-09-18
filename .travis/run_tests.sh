#!/bin/bash

set -xe

# Build scipp
mkdir -p build
mkdir -p install
cd build

if [[ "$TRAVIS_COMPILER" == "clang" ]]
then
    cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_CXX_FLAGS="-fsanitize=undefined" ..
else
    cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install ..
fi

make -j2 install all-tests

# Units tests
./units/test/scipp-units-test > log.txt 2> err.txt
echo "Units test"
cat err.txt
cat log.txt

# Core tests
./core/test/scipp-core-test > log.txt 2> err.txt
echo "Core test"
cat err.txt
cat log.txt

# Neutron tests
./neutron/test/scipp-neutron-test > log.txt 2> err.txt
echo "Neutron test"
cat err.txt
cat log.txt

# Python tests
python3 -m pip install -r ../python/requirements.txt
export PYTHONPATH=${PYTHONPATH}:../install
cd ../python
python3 -m pytest
