#!/bin/bash

set -ex

mkdir -p 'build' && cd 'build'

# Should benchmarks be built and executed
WITH_BENCHMARKS="${WITH_BENCHMARKS:-OFF}"

# Perform CMake configuration
cmake \
  -G Ninja \
  -DPYTHON_EXECUTABLE="$CONDA_PREFIX/bin/python" \
  -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_VERSION \
  -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_VERSION.sdk" \
  -DWITH_CTEST=OFF \
  -DWITH_BENCHMARKS="$WITH_BENCHMARKS" \
  ..

# Show cmake settings
cmake -B . -S .. -LA

# Build C++ tests and install Python package
ninja -v all-tests install

# Run C++ tests
./common/test/scipp-common-test
./units/test/scipp-units-test
./core/test/scipp-core-test
./variable/test/scipp-variable-test
./dataset/test/scipp-dataset-test
./neutron/test/scipp-neutron-test

# Build and run benchmarks if they were requested
if [ "$WITH_BENCHMARKS" != 'OFF' ]; then
  ninja -v all-benchmarks
  ./benchmark/dataset_benchmark
  ./benchmark/dataset_operations_benchmark
  ./benchmark/event_filter_benchmark
  ./benchmark/groupby_benchmark
  ./benchmark/histogram_benchmark
  ./benchmark/neutron_convert_benchmark
  ./benchmark/slice_benchmark
  ./benchmark/sparse_histogram_op_benchmark
  ./benchmark/transform_benchmark
  ./benchmark/variable_benchmark
fi

# Move scipp Python library to site packages location
mv "$CONDA_PREFIX/scipp" "$CONDA_PREFIX"/lib/python*/
