#!/bin/sh

# # Build scipp
# mkdir -p build
# mkdir -p install
# cd build
# cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DPYTHON_EXECUTABLE=/usr/bin/python3 -DCMAKE_INSTALL_PREFIX=../install ..
# make -j2 install
echo $(pwd)

# Units tests
./units/test/scipp-units-test

# Core tests
./core/test/scipp-core-test

# Neutron tests
# There are none so the executable cannot be built
#./neutron/test/scipp-neutron-test

# Python tests
python3 -m pip install -r ../scippy/requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
cd ../scippy
python3 -m pytest
