#!/bin/bash

set -xe

# Build scipp
mkdir -p build
mkdir -p install
cd build

SANITIZER_FLAGS=""

if [[ "$TRAVIS_COMPILER" == "clang" ]]
then
    SANITIZER_FLAGS=${SANITIZER_FLAGS}' -DCMAKE_CXX_FLAGS="-fsanitize=address"'
fi

echo "\n\n\n\${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install $SANITIZER_FLAGS .. \n\n\n"
cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install $SANITIZER_FLAGS ..
make -j2 install all-tests

# Units tests
./units/test/scipp-units-test

# Core tests
./core/test/scipp-core-test

# Neutron tests
./neutron/test/scipp-neutron-test

# Python tests
python3 -m pip install -r ../python/requirements.txt
export PYTHONPATH=${PYTHONPATH}:../install
cd ../python
python3 -m pytest
