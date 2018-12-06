from distributed.protocol import dask_serialize, dask_deserialize

from .dataset import *
from .xarray_compat import as_xarray

# For a distributed run, registering the serialiers for Dataset must happen somewhere that will cause them to be imported by all workers, for now in the __init__.py seems to work.
@dask_serialize.register(Dataset)
def serialize(dataset):
    header = {}
    frames = dataset.serialize()
    return header, frames

@dask_deserialize.register(Dataset)
def deserialize(header, frames):
    dataset = Dataset()
    # We serialize into `bytes` but somehow get back `bytearray`,
    # which pybind11 does not understand. Convert as workaround:
    dataset.deserialize([bytes(frame) for frame in frames])
    return dataset;
