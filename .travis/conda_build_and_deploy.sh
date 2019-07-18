#!/bin/sh

# Show this for debugging only
git describe --tags

# Build and upload
conda-build \
  --user 'scipp' \
  --token "$ANACONDA_TOKEN" \
  --label 'dev' \
  ./conda
