from .scippy import concatenate
import dask

from dask.base import DaskMethodsMixin, tokenize, dont_optimize
from dask.utils import funcname
import dask.array as da
from dask.array.core import top
from itertools import product
import operator
import uuid
from dask import sharedict
from dask.sharedict import ShareDict
from dask.context import globalmethod
from dask.array.optimization import optimize
from toolz import concat

# dask is trying to pickle Dataset.slice, which fails, wrapping it as a
# workaround.


def do_slice(dataset, slice_dim, i):
    return dataset.slice(slice_dim, i)


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
        return [(self.name,) + (i,) for i in range(self.n_chunk)]

    __dask_optimize__ = globalmethod(optimize, key='array_optimize',
                                     falsey=dont_optimize)
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
        assert slice_dim != self.slice_dim  # TODO implement
        return elemwise(do_slice, self, slice_dim, index)

    def concatenate(self, dim, other):
        assert dim != self.slice_dim  # TODO implement
        return elemwise(concatenate, dim, self, other)


def finalize(results, slice_dim):
    size = len(results)
    if size == 1:
        return results[0]
    left = finalize(results[:size // 2], slice_dim)
    right = finalize(results[size // 2:], slice_dim)
    return concatenate(slice_dim, left, right)


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
    out = '{}-{}'.format(funcname(op), tokenize(op, *args))
    out_ind = (0,)
    # Handling only 1D chunking here, so everything is (0,).
    arginds = list((a, (0,) if isinstance(a, DatasetCollection) else None)
                   for a in args)
    numblocks = {a.name: a.numblocks for a, ind in arginds if ind is not None}
    argindsstr = list(
        concat([(a if ind is None else a.name, ind) for a, ind in arginds]))
    dsk = top(op, out, out_ind, *argindsstr, numblocks=numblocks, **kwargs)
    dsks = [a.dask for a, ind in arginds if ind is not None]
    return DatasetCollection(sharedict.merge(
        (out, dsk), *dsks), out, n_chunk, slice_dim)


def from_dataset(dataset, slice_dim):
    # See also da.from_array.
    size = dataset.dimensions().size(slice_dim)
    # Dataset currently only supports slices with thickness 1.
    # TODO make correct chunk size based on dataset.dimensions(), so we can
    # handle multi-dimensional cases?
    chunks = da.core.normalize_chunks(1, shape=(size,))
    # Use a random name, similar to da.from_array with name = None.
    original_name = name = 'dataset-' + str(uuid.uuid1())
    # See also da.core.getem.
    keys = list(product([name], *[range(len(bds)) for bds in chunks]))
    values = [(do_slice, original_name, slice_dim, i) for i in range(size)]
    dsk = dict(zip(keys, values))
    dsk[original_name] = dataset
    return DatasetCollection(dsk, name, size, slice_dim)
