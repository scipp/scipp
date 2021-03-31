#!/bin/bash

set -ex

if test -z "${INSTALL_TARGET}"
then
  ./tools/make_and_install.sh
  INSTALL_TARGET=$CONDA_PREFIX
fi

# Move scipp Python library to site packages location
mv "$INSTALL_TARGET"/scipp "$CONDA_PREFIX"/lib/python*/
mv "$INSTALL_TARGET"/lib/*.so "$CONDA_PREFIX"/lib/
mv "$INSTALL_TARGET"/lib/cmake/scipp "$CONDA_PREFIX"/lib/cmake/
mv "$INSTALL_TARGET"/include/* "$CONDA_PREFIX"/include/
