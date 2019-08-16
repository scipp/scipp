[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE.txt)
[![Build Status](https://www.travis-ci.org/scipp/scipp.svg?branch=master)](https://www.travis-ci.org/scipp/scipp)
[![Documentation Status](https://readthedocs.org/projects/scipp/badge/?version=latest)](https://scipp.readthedocs.io/en/latest/?badge=latest)
[![Anaconda-Server Badge](https://anaconda.org/scipp/scipp/badges/installer/conda.svg)](https://conda.anaconda.org/scipp/label/dev)
[![Docker Cloud Build Status](https://img.shields.io/docker/cloud/build/scipp/scipp-jupyter-demo)](https://hub.docker.com/r/scipp/scipp-jupyter-demo)

# Scipp

See https://scipp.readthedocs.io on how to use and install the **scipp** Python module.

Scipp mainly provides the `Dataset` container, which is inspired by `xarray.Dataset`.

## Build instructions

It is not necessary to build `scipp` from source if only the Python package is required.
See https://scipp.readthedocs.io/en/latest/getting-started/installation.html on how to install using `conda` or `docker` instead.

### Prerequisites (OSX only)

#### XCode
XCode 10.2 or greater provides a `clang++` implementation with sufficient language support for `scipp`.

#### LLVM Clang
You will need to be using [LLVM Clang](https://releases.llvm.org/download.html) version 7 or greater. 

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

## Additional build options

1. `-DDYNAMIC_LIB` forces the shared libraries building, that also decreases link time.

For development purposes `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF -DDYNAMIC_LIB=ON` is recommended.
