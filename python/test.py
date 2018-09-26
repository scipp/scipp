#!/usr/bin/python3
# export PYTHONPATH=$PYTHONPATH:/home/simon/build/dataset/exports

from dataset import *
import dask
import numpy as np

from multiprocessing.pool import ThreadPool
from dask.distributed import Client

import timeit

from dask.base import DaskMethodsMixin
from dask.utils import funcname
import dask.array as da
from dask.array.core import top
from itertools import product
import operator
import uuid
from dask import sharedict
from dask.sharedict import ShareDict
from toolz import concat

class DatasetCollection(DaskMethodsMixin):
    def __init__(self, dask, name, n_chunk, slice_dim):
        if not isinstance(dask, ShareDict):
            s = ShareDict()
            s.update_with_key(dask, key=name)
            dask = s
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

    def __dask_tokenize__(self):
        return self.name

    @property
    def numblocks(self):
        return (self.n_chunk,)

    def __add__(self, other):
        return elemwise(operator.add, self, other)

    def slice(self, slice_dim, index):
        assert slice_dim != self.slice_dim # TODO implement
        return elemwise(Dataset.slice, self, slice_dim, index)

    def concatenate(self, dim, other):
        assert dim != self.slice_dim # TODO implement
        return elemwise(concatenate, dim, self, other)

def finalize(results, slice_dim):
    result = results[0]
    for r in results[1:]:
        result = concatenate(slice_dim, result, r)
    return result

def elemwise(op, *args, **kwargs):
    # See also da.core.elemwise. Note: dask seems to be able to convert Python
    # and numpy objects in this function, thus supporting operations between
    # dask objects and others. This would be useful for us as well.
    # Do not support mismatching chunking for now.
    n_chunk = None
    slice_dim = None
    for arg in args:
        if isinstance(arg, DatasetCollection):
            if n_chunk is not None:
                assert n_chunk == arg.n_chunk
                assert slice_dim == arg.slice_dim
            else:
                n_chunk = arg.n_chunk
                slice_dim = arg.slice_dim
    argstring = ','.join(['{}'.format(arg.name if isinstance(arg, DatasetCollection) else arg ) for arg in args])
    out = '{}({})'.format(funcname(op), argstring)
    out_ind = (0,)
    # Handling only 1D chunking here, so everything is (0,).
    arginds = list((a, (0,) if isinstance(a, DatasetCollection) else None) for a in args)
    numblocks = {a.name: a.numblocks for a, ind in arginds if ind is not None}
    argindsstr = list(concat([(a if ind is None else a.name, ind) for a, ind in arginds]))
    dsk = top(op, out, out_ind, *argindsstr, numblocks=numblocks, **kwargs)
    dsks = [a.dask for a, ind in arginds if ind is not None]
    return DatasetCollection(sharedict.merge((out, dsk), *dsks), out, n_chunk, slice_dim)

def from_dataset(dataset, slice_dim):
    # See also da.from_array.
    size = dataset.dimensions().size(slice_dim)
    # Dataset currently only supports slices with thickness 1.
    # TODO make correct chunk size based on dataset.dimensions(), so we can handle multi-dimensional cases?
    chunks = da.core.normalize_chunks(1, shape=(size,))
    # Use a random name, similar to da.from_array with name = None.
    original_name = name = 'dataset-' + str(uuid.uuid1())
    # See also da.core.getem.
    keys = list(product([name], *[range(len(bds)) for bds in chunks]))
    # dask is trying to pickle Dataset.slice, which fails, wrapping it as a workaround.
    def do_slice(dataset, slice_dim, i):
        return dataset.slice(slice_dim, i)
    values = [ (do_slice, original_name, slice_dim, i) for i in range(size) ]
    dsk = dict(zip(keys, values))
    dsk[original_name] = dataset
    return DatasetCollection(dsk, name, size, slice_dim)

if __name__ == '__main__':
    with Client(n_workers=3) as client:
        lx = 2
        ly = 3
        lz = 10
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

        d.insert(Coord.X, dimsX, range(lx))
        d.insert(Coord.Y, dimsY, range(ly))
        d.insert(Coord.Z, dimsZ, range(lz))
        d.insertDataValue("name", dims, np.arange(lx*ly*lz))

        volume = lx*ly*lz
        print("Dataset volume is {} ({} GByte)".format(volume, (volume*8)/2**30))


        test = from_dataset(d, slice_dim=Dimension.Z)
        test = test.concatenate(Dimension.X, test)
        test = test.persist()
        test = test.compute()
        for val in test.getDataValue():
            print(val)
        print(test.size())

    #start_time = timeit.default_timer()
    #tmp = d.slice(Dimension.X, 7)
    #print(timeit.default_timer() - start_time)

    #with dask.config.set(pool=ThreadPool(10)):
    #    test = from_dataset(d, slice_dim=Dimension.Z)
    #    test = test.persist()

    #    start_time = timeit.default_timer()
    #    sliced = test.slice(Dimension.X, 7)
    #    sliced = sliced.compute()
    #    print(timeit.default_timer() - start_time)

    #    start_time = timeit.default_timer()
    #    sliced = test.slice(Dimension.Y, 111)
    #    sliced = sliced.compute()
    #    print(timeit.default_timer() - start_time)

    #    #sliced.visualize(filename='test.svg')

    #start_time = timeit.default_timer()
    #d2 = d + d
    #for i in range(10):
    #    d2 += d
    #print(timeit.default_timer() - start_time)

    #with dask.config.set(pool=ThreadPool(10)):
    #    d = from_dataset(d, slice_dim=Dimension.Z)
    #    #d = d.persist() # executes the slicing

    #    d3 = d + d
    #    for i in range(10):
    #        d3 = d3 + d

    #    #d3.visualize(filename='test.svg')

    #    print('computing...')
    #    start_time = timeit.default_timer()
    #    d3 = d3.persist()
    #    elapsed = timeit.default_timer() - start_time
    #    print('{} {} GB/s'.format(elapsed, lx*ly*lz*8*4*11/elapsed/2**30))
