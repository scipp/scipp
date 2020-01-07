#!/bin/bash
set -x

mkdir -p 'build' && cd 'build'

# Perform CMake configuration
cmake \
  -G"$GENERATOR" \
  -DPYTHON_EXECUTABLE="$CONDA_PREFIX/bin/python" \
  -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_VERSION \
  -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_VERSION.sdk" \
  -DWITH_CTEST=OFF \
  ..

# Catch errors in CMake configuration step
rc=$?
if [ $rc -ne 0 ]; then
  exit $rc
fi
# Show cmake settings
cmake -B . -S .. -LA

cmake --build . --target scipp-common-test -- -v 
cmake --build . --target scipp-core-test -- -v 
cmake --build . --target scipp-units-test -- -v 
cmake --build . --target scipp-neutron-test -- -v 
# C++ tests
./common/test/scipp-common-test
./units/test/scipp-units-test
./core/test/scipp-core-test
./neutron/test/scipp-neutron-test

# Build, install and move scipp Python library to site packages location
cmake --build . --target install -- -v && mv "$CONDA_PREFIX/scipp" "$CONDA_PREFIX"/lib/python*/
