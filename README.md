[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE.txt)
[![Build Status](https://www.travis-ci.org/scipp/scipp.svg?branch=master)](https://www.travis-ci.org/scipp/scipp)
[![Documentation Status](https://readthedocs.org/projects/scipp/badge/?version=latest)](https://scipp.readthedocs.io/en/latest/?badge=latest)
[![Anaconda-Server Badge](https://anaconda.org/scipp/scipp/badges/installer/conda.svg)](https://conda.anaconda.org/scipp)

# Scipp

See https://scipp.readthedocs.io for documentation of **scipp**.

Scipp mainly provides the `Dataset` container, which is inspired by `xarray.Dataset`.

## Build instructions

### Prerequisites (OSX only)

* You will need to be running High Sierra 10.13. Lower version have incompatible libc++ implementations.
* You will need to be using [LLVM Clang](https://releases.llvm.org/download.html) version 6 or greater. Current latest XCode 10.1, does not support all language features used. Note that pybind11's use of `std::variant` presents the current issues for `Apple LLVM 10.0.0`. Pybind11 automatically picks up on the CMAKE_CXX_STANDARD 17 applied in the dataset configuration and presumes to use `std::variant`.
* You will need to `brew install libomp`.

### Getting the code, building, and installing

To build and install the library:

```
git submodule init
git submodule update
mkdir -p build
mkdir -p install
cd build
cmake -DPYTHON_EXECUTABLE=/usr/bin/python3 -DCMAKE_INSTALL_PREFIX=../install ..
make -j4 install
```

To use the `scipp` Python module:

```
cd ../python
python3 -m pip install -r requirements.txt
export PYTHONPATH=$PYTHONPATH:../install
```

You should use Python 3.5 or greater

In Python:

```python
import scipp as sc
```

## Running the unit tests

To run the C++ tests, run (in directory `build/`):
```sh
./units/test/scipp-units-test
./core/test/scipp-core-test
```

Note that simply running `ctest` also works, but currently it seems to have an issue with gathering templated tests, so calling the test binaries manually is recommended (and much faster).

To run the Python tests, run (in directory `python/`):

```sh
cd python
python3 -m pip install -r requirements.txt
python3 -m pytest
```

## Running the demo notebooks

The demo/tutorial notebooks are using a plugin from `nbextension`, which needs to be installed separately:

```sh
pip install jupyter_contrib_nbextensions
jupyter contrib nbextension install --user

pip install jupyter_nbextensions_configurator
jupyter nbextensions_configurator enable --user
```

Use `pip3`, depending on your environment.
