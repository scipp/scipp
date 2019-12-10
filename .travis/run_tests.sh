#!/bin/bash

set -xe

# Build scipp and test ONLY if not deploying, i.e. not master and a pull_request event
if [[$TRAVIS_BRANCH -ne 'master'] && [$TRAVIS_EVENT_TYPE -eq 'pull_request']]
then
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
else
    echo 'build and test step skipped on' $TRAVIS_BRANCH
fi

