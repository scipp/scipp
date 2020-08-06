#!/bin/bash

set -ex

mkdir -p 'build' && cd 'build'

if test -z "${OSX_VERSION}"
then
  IPO="ON"
else
  # Disable IPO for OSX due to unexplained linker failure.
  IPO="OFF"
fi

# Perform CMake configuration
cmake \
  -G Ninja \
  -DPYTHON_EXECUTABLE="$CONDA_PREFIX/bin/python" \
  -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_VERSION \
  -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_VERSION.sdk" \
  -DWITH_CTEST=OFF \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=$IPO \
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
