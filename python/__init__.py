from distributed.protocol import dask_serialize, dask_deserialize

from .dataset import *

# For a distributed run, registering the serialiers for Dataset must happen somewhere that will cause them to be imported by all workers, for now in the __init__.py seems to work.
@dask_serialize.register(Dataset)
def serialize(dataset):
    header = {}
    frames = dataset.__getstate__()
    #print("serializing {} vars".format(len(frames)))
    #print(frames)
    return header, frames

@dask_deserialize.register(Dataset)
def deserialize(header, frames):
    dataset = Dataset()
    #print("deserializing {} vars".format(len(frames)))
    #print(frames)
    dataset.deserialize([bytes(frame) for frame in frames])
    #dataset.deserialize(frames)
    return dataset;
