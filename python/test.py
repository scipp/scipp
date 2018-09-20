#!/usr/bin/python3
# export PYTHONPATH=$PYTHONPATH:/home/simon/build/dataset/exports

from dataset import *
import dask
import numpy as np

from multiprocessing.pool import ThreadPool
from dask.distributed import Client

import timeit

from dask.base import DaskMethodsMixin
#from dask.array.core import normalize_chunks
#from dask import ShareDict
from dask.utils import funcname
from dask.array.core import top
from itertools import product
import operator

class DatasetCollection(DaskMethodsMixin):
    def __init__(self, dask, name, n_chunk, slice_dim):
        self.dask = dask
        self.name = name
        self.n_chunk = n_chunk
        self.slice_dim = slice_dim

    def __dask_graph__(self):
        return self.dask

    def __dask_keys__(self):
        return [ (self.name,) + (i,) for i in range(self.n_chunk) ]

    __dask_scheduler__ = staticmethod(dask.threaded.get)

    def __dask_postcompute__(self):
        return finalize, (self.slice_dim, )

    def __dask_postpersist__(self):
        return DatasetCollection, (self.name, self.n_chunk, self.slice_dim)

    def __add__(self, other):
        return elemwise(operator.add, self, other)

def finalize(results, slice_dim):
    result = results[0]
    for r in results[1:]:
        result = concatenate(slice_dim, result, r)
    return result

def elemwise(op, a, b):
    assert a.n_chunk == b.n_chunk
    assert a.slice_dim == b.slice_dim
    name = "{}({},{})".format(funcname(op), a.name, b.name)
    dsk = top(op, name, 'i', a.name, 'i', b.name, 'i', numblocks={a.name : (a.n_chunk,), b.name : (b.n_chunk,)})
    dask = {**a.dask, **b.dask, **dsk}
    return DatasetCollection(dask, name, a.n_chunk, a.slice_dim)

def from_dataset(dataset, slice_dim):
    name = "dummy"
    size = dataset.dimensions().size(slice_dim)
    keys = [ (name, i) for i in range(size) ]
    values = [ (Dataset.slice, name, slice_dim, i) for i in range(size) ]
    dsk = dict(zip(keys, values))
    dsk[name] = dataset
    return DatasetCollection(dsk, name, size, slice_dim)

lx = 2000
ly = 2000
lz = 100
d = Dataset()
dimsX = Dimensions()
dimsX.add(Dimension.X, lx)
dimsY = Dimensions()
dimsY.add(Dimension.Y, ly)
dimsZ = Dimensions()
dimsZ.add(Dimension.Z, lz)
dims = Dimensions()
dims.add(Dimension.X, lx)
dims.add(Dimension.Y, ly)
dims.add(Dimension.Z, lz)

d.insertCoordX(dimsX, range(lx))
d.insertCoordY(dimsY, range(ly))
d.insertCoordZ(dimsZ, range(lz))
d.insertDataValue("name", dims, np.arange(lx*ly*lz))

d2 = d + d
d2 += d
#with dask.config.set(pool=ThreadPool(4)):
#dask.compute(PG3_4844)

orig = d

start_time = timeit.default_timer()
d2 += d
#orig = orig + orig
elapsed = timeit.default_timer() - start_time
print(elapsed)

d = from_dataset(d, slice_dim=Dimension.Z)
d = d.persist()
d2 = d + d
for i in range(10):
    d2 = d2 + d

#d2.visualize(filename='test.svg')
print('computing...')
start_time = timeit.default_timer()
d2 = d2.persist()
elapsed = timeit.default_timer() - start_time
print(elapsed)
#d2 = d2.compute()
#print(orig + orig + orig == d2)
