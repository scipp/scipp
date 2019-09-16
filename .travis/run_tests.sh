#!/bin/bash

set -xe

# Build scipp
mkdir -p build
mkdir -p install
cd build

SANITIZER_FLAGS=""

if [[ "$TRAVIS_COMPILER" == "clang" ]]
then
    SANITIZE_SHARED="\"-fsanitize=address -shared-libasan\""
    SANITIZER_FLAGS=${SANITIZER_FLAGS}" -DCMAKE_CXX_FLAGS="$SANITIZE_SHARED"
fi

cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} $@ -DCMAKE_INSTALL_PREFIX=../install ${SANITIZER_FLAGS} ..
make -j2 install all-tests

if [[ "$TRAVIS_COMPILER" == "clang" ]]
then
    export LD_PRELOAD=$(find / -name libclang_rt.asan-x86_64.so)
    export ASAN_OPTIONS=detect_odr_violation=0
fi

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
