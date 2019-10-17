#!/bin/bash

set -xe

# docker pull nvaytet/scipp-ci
# - docker run -d -p 127.0.0.1:80:4567 carlad/sinatra /bin/sh -c "cd /root/sinatra; bundle exec foreman start;"
# - docker ps -a

# C++17
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -qq clang-format-6.0 python3-pip ninja-build
python3 -m pip install flake8
