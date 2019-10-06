#!/bin/bash

set -xe

# C++17
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
pyenv global system 3.7
sudo apt-get install -qq clang-format-5.0 python3-pip ninja-build
python3 -m pip install --upgrade pip
python3 -m pip install flake8
