#!/bin/bash

set -ex

mkdir -p 'build' && cd 'build'

# Perform CMake configuration
cmake \
  -G Ninja \
  -DPYTHON_EXECUTABLE="$CONDA_PREFIX/bin/python" \
  -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_VERSION \
  -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_VERSION.sdk" \
  -DWITH_CTEST=OFF \
  ..

# Show cmake settings
cmake -B . -S .. -LA

# Build benchmarks, C++ tests and install Python package
ninja -v all-benchmarks all-tests install

# Run C++ tests
./common/test/scipp-common-test
./units/test/scipp-units-test
./core/test/scipp-core-test
./variable/test/scipp-variable-test
./dataset/test/scipp-dataset-test
./neutron/test/scipp-neutron-test

# Move scipp Python library to site packages location
mv "$CONDA_PREFIX/scipp" "$CONDA_PREFIX"/lib/python*/
