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

# Build, install and move scipp Python library to site packages location
cmake --build . --target install && \
  mv "$CONDA_PREFIX/scipp" "$CONDA_PREFIX"/lib/python*/
