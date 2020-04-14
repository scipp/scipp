#!/bin/bash

SANITIZER=$1

if [[ "$#" -eq 2 ]]; then
  SOURCE=$2
else
  SOURCE=$(cd $(dirname $BASH_SOURCE); pwd)/..
fi

WORKING_DIR=$PWD
BUILD=${WORKING_DIR}/build-sanitizer
INSTALL=${WORKING_DIR}/install-sanitizer

cd ${WORKING_DIR}
mkdir ${BUILD}
mkdir ${INSTALL}
cd ${BUILD}
# We need to disable ctest integration of gtest-based tests since gtest_discover_tests
# fails due to missing (asan) preload.
cmake -DWITH_CTEST=Off -DSANITIZE_${SANITIZER}=On -DCMAKE_INSTALL_PREFIX=${INSTALL} ${SOURCE}
make -j
make -j all-tests
make install
export ASan_WRAPPER=${SOURCE}/CMake/sanitizers-cmake/cmake/asan-wrapper
${ASan_WRAPPER} ${BUILD}/common/test/scipp-common-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/units/test/scipp-units-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/core/test/scipp-core-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/variable/test/scipp-variable-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/dataset/test/scipp-dataset-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/neutron/test/scipp-neutron-test || { exit 1; }
cd ${SOURCE}/python
export PYTHONPATH=${PYTHONPATH}:${INSTALL}
python3 -m pytest || { exit 1; }

cd ${WORKING_DIR}
rm -rf ${BUILD} ${INSTALL}
