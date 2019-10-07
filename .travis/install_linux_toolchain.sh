#!/bin/bash

set -xe

# C++17
# sudo apt-get install -qq g++-7 libomp-dev python3-setuptools ipython3
sudo apt-get install -qq g++-7 libomp-dev
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 90
