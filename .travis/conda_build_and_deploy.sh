#!/bin/bash

set -x

# Build and upload
conda-build \
  --user 'scipp' \
  --token "$ANACONDA_TOKEN" \
  --channel conda-forge \
  --no-remove-work-dir \
  $@ \
  ./conda
