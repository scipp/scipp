#!/bin/bash

set -xe

# Download Miniconda installer
wget "https://repo.continuum.io/miniconda/$1" -O 'miniconda.sh'

# Install Miniconda
bash 'miniconda.sh' -b -p "$HOME/miniconda"
conda config \
  --set always_yes yes \
  --set changeps1 no
conda update -q conda
