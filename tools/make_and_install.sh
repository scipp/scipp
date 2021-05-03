#!/bin/bash

set -ex

wget https://raw.githubusercontent.com/scipp/pipelines/main/make.sh
bash make.sh

# Run C++ tests
./bin/scipp-common-test
./bin/scipp-units-test
./bin/scipp-core-test
./bin/scipp-variable-test
./bin/scipp-dataset-test
