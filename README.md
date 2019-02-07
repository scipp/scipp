# libDataset

This a prototype for the `Dataset` library.
`Dataset` is inspired by `xarray.Dataset` and could replace most of the workspace types available in Mantid while at the same time providing more flexibility and features.

## Build instructions

```
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make
```

### OSX
* You will need to be running High Sierra 10.13. Lower version have incompatible libc++ implementations.
* You will need to be using LLVM Clang version 7. Current latest XCode 10.1, does not support all language features used.  
* You will need to `brew install libomp` 

## Usage of the Python exports

Setup is as above, but the `install` target needs to be run to setup the Python files:

```
cmake -DCMAKE_INSTALL_PREFIX=/some/path ..
make install
```

Then, add the install location `/some/path` to `PYTHONPATH`.
You can now do, e.g.,

```python
from dataset import Dataset
```

## Running the unit tests

To run the Python tests, run the following in directory `python/`:

```sh
python3 -m unittest discover test
```
or via nose
```
nosetests --where test
```

Note that the tests bring in additional python dependencies. `python3 -m pip install -r python/requirements.txt` to obtain these.
