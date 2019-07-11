#!/bin/sh

# Download Miniconda installer
wget 'https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh' -O 'miniconda.sh'

# Install Miniconda
bash 'miniconda.sh' -b -p "$HOME/miniconda"

# Install build and deploy dependencies
conda install --yes conda-build anaconda
