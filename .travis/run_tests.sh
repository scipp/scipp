#!/bin/sh

# Build scipp (exit early if either the configure or build step fails)
mkdir -p build
mkdir -p install
cd build
cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install .. || exit 1
make -j2 all-tests install || exit 1

# Units tests
./units/test/scipp-units-test

# Core tests
./core/test/scipp-core-test

# Neutron tests
./neutron/test/scipp-neutron-test

# Python tests (exit early if dependencies could not be installed)
python3 -m pip install -r ../python/requirements.txt || exit 1
export PYTHONPATH=$PYTHONPATH:../install
cd ../python
python3 -m pytest
