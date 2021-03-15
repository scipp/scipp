#!/bin/bash

set -ex

if test -z "${SKIP_BUILD}"
then
  ./tools/make_and_install.sh
fi

# Move scipp Python library to site packages location
mv "$CONDA_PREFIX/scipp" "$CONDA_PREFIX"/lib/python*/
