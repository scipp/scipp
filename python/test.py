#!/usr/bin/python3
# export PYTHONPATH=$PYTHONPATH:/home/simon/build/dataset/exports

from dataset import *
from dataset.dataset_collection import from_dataset
import dask
import numpy as np

from multiprocessing.pool import ThreadPool
from dask.distributed import Client

import timeit

from dask.distributed import wait

if __name__ == '__main__':
    with Client(n_workers=10, serializers=['dask'], deserializers=['dask']) as client:
    #with Client('localhost:8786', n_workers=10, serializers=['dask'], deserializers=['dask']) as client:
        # 10, 1000, 100
        lx = 100
        ly = 1000
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

        # Note: Axes add significant overhead for serialization, etc., slicing is 30% faster without them.
        d.insert(Coord.X, dimsX, range(lx))
        d.insert(Coord.Y, dimsY, range(ly))
        d.insert(Coord.Z, dimsZ, range(lz))
        d.insertDataValue("name", dims, np.arange(lx*ly*lz))

        volume = lx*ly*lz
        print("Dataset volume is {} ({} GByte)".format(volume, (volume*8)/2**30))

        test = from_dataset(d, slice_dim=Dimension.Z)
        test = client.persist(test)
        test = test.concatenate(Dimension.Y, test)
        test = test.concatenate(Dimension.Y, test)
        test = test.concatenate(Dimension.Y, test)
        test = test.concatenate(Dimension.Y, test)
        test = test.concatenate(Dimension.Y, test)
        test = client.persist(test)
        #test = test.persist()
        wait(test)

        start_time = timeit.default_timer()
        sliced = test.slice(Dimension.Y, 6)
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        start_time = timeit.default_timer()
        sliced = test.slice(Dimension.X, 7)
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        start_time = timeit.default_timer()
        sliced = test.slice(Dimension.X, 8)
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        start_time = timeit.default_timer()
        sliced = test.slice(Dimension.X, 6)
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        print(len(sliced.getDataValue()))

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
