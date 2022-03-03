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
cmake -DSANITIZE_${SANITIZER}=On -DCMAKE_INSTALL_PREFIX=${INSTALL} ${SOURCE}
make -j
make -j all-tests
make install
export ASan_WRAPPER=${SOURCE}/lib/cmake/sanitizers-cmake/cmake/asan-wrapper
${ASan_WRAPPER} ${BUILD}/bin/scipp-common-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/bin/scipp-units-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/bin/scipp-core-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/bin/scipp-variable-test || { exit 1; }
${ASan_WRAPPER} ${BUILD}/bin/scipp-dataset-test || { exit 1; }
cd ${SOURCE}/test
export PYTHONPATH=${INSTALL}
export LD_PRELOAD=$(ldd ${INSTALL}/scipp/_scipp*.so | grep libasan | sed "s/^[[:space:]]//" | cut -d' ' -f1)
python3 -m pytest -s || { exit 1; }

cd ${WORKING_DIR}
rm -rf ${BUILD} ${INSTALL}
