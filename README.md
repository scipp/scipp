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
