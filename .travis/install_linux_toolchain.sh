#!/bin/bash

set -xe

# C++17
sudo apt-get install -qq g++-7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 90
sudo apt install libomp-dev
sudo apt install python3-setuptools
sudo apt install ipython3
