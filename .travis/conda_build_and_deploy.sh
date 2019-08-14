#!/bin/sh

# Build and upload
conda-build \
  --user 'scipp' \
  --token "$ANACONDA_TOKEN" \
  --label 'dev' \
  ./conda
