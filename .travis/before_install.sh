#!/bin/bash

set -xe

# C++17
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
pyenv global system 3.7
# sudo apt-get install -qq clang-format-5.0 ninja-build
# .travis/install_conda.sh "Miniconda3-latest-Linux-x86_64.sh"
# conda install flake8
sudo apt-get install -qq clang-format-5.0 python3-pip ninja-build
# sudo pip3 install --upgrade pip
python3 -m pip install flake8
