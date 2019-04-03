#!/bin/sh

# C++17
sudo apt-get install -qq g++-7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 90
sudo add-apt-repository -y ppa:mhier/libboost-latest
sudo apt update
sudo apt install libboost1.67-dev
sudo apt install libomp-dev
sudo apt install python3-setuptools
sudo apt install python3-pip
sudo apt install ipython3
