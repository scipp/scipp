[![Build Status](https://www.travis-ci.org/scipp/scipp.svg?branch=master)](https://www.travis-ci.org/scipp/scipp)
[![Documentation Status](https://readthedocs.org/projects/scipp/badge/?version=latest)](https://scipp.readthedocs.io/en/latest/?badge=latest)

# Scipp

See https://scipp.readthedocs.io for documentation of **scipp*.

Scipp mainly provides the `Dataset` container, which is inspired by `xarray.Dataset`.

## Build instructions

### Prerequisites

We require a minimum of `boost1.67`.

#### Ubuntu 18.04
On Ubuntu 18.04 the maximum available is `boost1.65`.
A newer version can be installed as follows:

```
cd path/to/new/boost
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz
tar -xzvf boost_1_69_0.tar.gz
```

#### OSX
* You will need to be running High Sierra 10.13. Lower version have incompatible libc++ implementations.
* You will need to be using [LLVM Clang](https://releases.llvm.org/download.html) version 6 or greater. Current latest XCode 10.1, does not support all language features used. Note that pybind11's use of `std::variant` presents the current issues for `Apple LLVM 10.0.0`. Pybind11 automatically picks up on the CMAKE_CXX_STANDARD 17 applied in the dataset configuration and presumes to use `std::variant`.
* You will need to `brew install libomp`.

### Getting the code and building

```
git submodule init
git submodule update
mkdir build
cd build
cmake -DBOOST_ROOT:PATHNAME=path/to/new/boost/boost_1_69_0 ..
make
```

## Usage of the Python exports

Setup is as above, but the `install` target needs to be run to setup the Python files:

```
cmake -DCMAKE_INSTALL_PREFIX=/some/path ..
make install
```

Then, add the install location `/some/path` to `PYTHONPATH`.
You can now do, e.g.,

```python
import scippy as sc
```

## Running the unit tests

To run the Python tests, run the following in directory `scippy/`:

```sh
python3 -m unittest discover test
```
or via nose
```
nosetests3 --where test
```

Note that the tests bring in additional python dependencies. `python3 -m pip install -r scippy/requirements.txt` to obtain these.

## Running the demo notebooks

The demo/tutorial notebooks are using a plugin from `nbextension`, which needs to be installed separately:

```sh
pip install jupyter_contrib_nbextensions
jupyter contrib nbextension install --user

pip install jupyter_nbextensions_configurator
jupyter nbextensions_configurator enable --user
```

Use `pip3`, depending on your environment.
