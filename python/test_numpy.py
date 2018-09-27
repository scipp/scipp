#!/usr/bin/python3

import dask.array as da
import numpy as np

from multiprocessing.pool import ThreadPool
from dask.distributed import Client

import timeit

from dask.distributed import wait

if __name__ == '__main__':
    # Slice X-slicing is almost 2x faster with this client, as opposed to client using workers launched by dask-ssh.
    #with Client(n_workers=10, serializers=['dask'], deserializers=['dask']) as client:
    with Client('localhost:8786', n_workers=10, serializers=['dask'], deserializers=['dask']) as client:
        lx = 100
        ly = 1000
        lz = 100
        # Note different dimension ordering in numpy compared to Dataset
        a = da.from_array(np.zeros((lz, ly, lx)), chunks=(1, -1, -1))
        print(a)

        volume = lx*ly*lz
        print("Dataset volume is {} ({} GByte)".format(volume, (volume*8)/2**30))

        a = client.persist(a)
        a = da.concatenate([a,a], axis=1)
        a = da.concatenate([a,a], axis=1)
        a = da.concatenate([a,a], axis=1)
        a = da.concatenate([a,a], axis=1)
        a = da.concatenate([a,a], axis=1)
        # da.concatenate does not merge chunks, even if they span the entire dimension, need to rechunk:
        a = a.rechunk(chunks=(1, -1, -1))
        a = client.persist(a)
        print(a.shape)
        wait(a)

        # Slice in Z (fast)
        start_time = timeit.default_timer()
        sliced = a[7,:,:]
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        # Slice in Y (fast)
        start_time = timeit.default_timer()
        sliced = a[:,7,:]
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        # Slice in X (extremely slow, why?)
        start_time = timeit.default_timer()
        sliced = a[:,:,7]
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        # Slice in X
        start_time = timeit.default_timer()
        sliced = a[:,:,8]
        sliced = sliced.compute()
        print(timeit.default_timer() - start_time)

        print(sliced.shape)
