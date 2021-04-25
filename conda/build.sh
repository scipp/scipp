#!/bin/bash

set -ex

if test -z "${INSTALL_PREFIX}"
then
  export INSTALL_PREFIX="$(pwd)/scipp"
  ./tools/make_and_install.sh
fi

mv "$INSTALL_PREFIX"/scipp "$CONDA_PREFIX"/lib/python*/
mv "$INSTALL_PREFIX"/lib/libscipp* "$CONDA_PREFIX"/lib/
mv "$INSTALL_PREFIX"/lib/libunits-shared.* "$CONDA_PREFIX"/lib/
mv "$INSTALL_PREFIX"/lib/cmake/scipp "$CONDA_PREFIX"/lib/cmake/
mv "$INSTALL_PREFIX"/include/Eigen "$CONDA_PREFIX"/include/
mv "$INSTALL_PREFIX"/include/scipp* "$CONDA_PREFIX"/include/
mv "$INSTALL_PREFIX"/include/units "$CONDA_PREFIX"/include/
