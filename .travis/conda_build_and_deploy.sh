#!/bin/sh

# Do not perform Conda build and publish unless this is a build of the master branch.
# Continue regardless if this script is run outside of a Travis-CI environment
if [[ $TRAVIS == 'true' ]] && [[ ! $TRAVIS_BRANCH == 'master' ]]; then
  echo 'Not performing conda build and publish for non-master branch job'
  exit
fi
echo 'Performing conda build and publish'

# Build and upload
conda-build \
  --user 'scipp' \
  --token "$ANACONDA_TOKEN" \
  --label 'dev' \
  ./conda
