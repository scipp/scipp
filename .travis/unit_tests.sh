#!/bin/bash

set -xe

# Checkout the version of scipp that matches the PR
cd /opt/scipp/build
git fetch origin +refs/pull/${1}/merge
git checkout -qf FETCH_HEAD
git branch
make -j2 install all-tests all-benchmarks

# Units tests
./units/test/scipp-units-test

# Core tests
./core/test/scipp-core-test

# Neutron tests
./neutron/test/scipp-neutron-test

# Python tests
python3 -m pip install -r ../python/requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
cd ../python
python3 -m pytest
