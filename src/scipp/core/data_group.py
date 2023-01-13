# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

import functools
import itertools
import numbers
import operator
from collections.abc import MutableMapping
from typing import Any, Callable, Iterable, overload

from ..typing import ScippIndex
from .cpp_classes import DataArray, Dataset, DimensionError, Variable


def _item_dims(item):
    return getattr(item, 'dims', ())


def _summarize(item):
    if isinstance(item, DataGroup):
        return f'{type(item).__name__}({len(item)}, {item.sizes})\n'
    if hasattr(item, 'sizes'):
        return f'{type(item).__name__}({item.sizes})\n'
    return str(item)


def _is_positional_index(key) -> bool:

    def is_int(x):
        return isinstance(x, numbers.Integral)

    if is_int(key):
        return True
    if isinstance(key, slice):

        if is_int(key.start) or is_int(key.stop) or is_int(key.step):
            return True
        if key.start is None and key.stop is None and key.step is None:
            return True
    return False


class DataGroup(MutableMapping):
    """
    A dict-like group of data. Additionally provides dims and shape properties.

    DataGroup acts like a Python dict but additionally supports Scipp functionality
    such as positional- and label-based indexing and Scipp operations by mapping them
    to the values in the dict. This may happen recursively to support tree-like data
    structures.

    .. versionadded:: 23.01.0
    """

    def __init__(self, /, *args, **kwargs):
        self._items = dict(*args, **kwargs)
        if not all([isinstance(k, str) for k in self._items.keys()]):
            raise ValueError("DataGroup keys must be strings.")

    def __len__(self) -> int:
        """Return the number of items in the data group."""
        return len(self._items)

    def __iter__(self):
        yield from self._items

    @overload
    def __getitem__(self, name: str) -> Any:
        ...

    @overload
    def __getitem__(self, name: ScippIndex) -> DataGroup:
        ...

    def __getitem__(self, name):
        """Return item of given name or index all items.

        When ``name`` is a string, return the item of the given name. Otherwise, this
        returns a new DataGroup, with items created by indexing the items in this
        DataGroup. This may perform, e.g., Scipp's positional indexing or label-based
        indexing on items that are scipp.Variable or scipp.DataArray.

        Positional indexing is only possible when the shape of all items is consistent
        for the indexed dimension.

        Label-based indexing is only possible when all items have a coordinate for the
        indexed dimension.
        """
        if isinstance(name, str):
            return self._items[name]
        if isinstance(name, tuple) and name == ():
            return DataGroup({key: var[()] for key, var in self.items()})
        if _is_positional_index(name):
            if self.ndim != 1:
                raise DimensionError(
                    "Slicing with implicit dimension label is only possible "
                    f"for 1-D objects. Got {self.sizes} with ndim={self.ndim}. Provide "
                    "an explicit dimension label, e.g., var['x', 0] instead of var[0].")
            dim = self.dims[0]
            index = name
        else:
            dim, index = name
        if _is_positional_index(index) and self.sizes[dim] is None:
            raise DimensionError(
                f"Positional indexing dim '{dim}' not possible as the length is not "
                "unique.")
        return DataGroup({
            key: var[dim, index] if dim in _item_dims(var) else var
            for key, var in self.items()
        })

    @overload
    def __setitem__(self, name: str, value: Any):
        ...

    def __setitem__(self, name, value):
        """Set self[key] to value."""
        self._items[name] = value

    def __delitem__(self, name: str):
        """Delete self[key]."""
        del self._items[name]

    @property
    def dims(self):
        """Union of dims of all items. Non-Scipp items are handled as dims=()."""
        dims = ()
        for var in self.values():
            # Preserve insertion order
            for dim in _item_dims(var):
                if dim not in dims:
                    dims = dims + (dim, )
        return dims

    @property
    def ndim(self):
        """Number of dimensions, i.e., len(self.dims)."""
        return len(self.dims)

    @property
    def shape(self):
        """Union of shape of all items. Non-Scipp items are handled as shape=()."""

        def dim_size(dim):
            sizes = {var.sizes[dim] for var in self.values() if dim in _item_dims(var)}
            if len(sizes) == 1:
                return next(iter(sizes))
            return None

        return tuple(dim_size(dim) for dim in self.dims)

    @property
    def sizes(self):
        """Dict combining dims and shape, i.e., mapping dim labels to their size."""
        return dict(zip(self.dims, self.shape))

    def _make_html(self):
        out = ''
        for name, item in self.items():
            if isinstance(item, DataGroup):
                html = item._make_html()
            elif isinstance(item, (Variable, DataArray, Dataset)):
                html = ''
            else:
                html = str(item)
            out += f"<details style=\"padding-left:2em\"><summary>"\
                   f"{name}: {_summarize(item)}</summary>{html}</details>"
        return out

    def _repr_html_(self):
        out = ''
        out += f"<details open=\"open\"><summary>DataGroup"\
               f"({len(self)})</summary>"
        out += self._make_html()
        out += "</details>"
        return out

    def __repr__(self):
        r = 'DataGroup(\n'
        for name, var in self.items():
            r += f'    {name}: {_summarize(var)}\n'
        r += ')'
        return r

    def __eq__(self, other):
        return _data_group_binary(operator.eq, self, other)

    def __add__(self, other):
        return _data_group_binary(operator.add, self, other)

    def __mul__(self, other):
        return _data_group_binary(operator.mul, self, other)

    def _call_method(self, func: Callable) -> DataGroup:
        """Call method on all values and return new DataGroup containing the results."""
        return DataGroup({key: func(value) for key, value in self.items()})

    @property
    def bins(self):
        # TODO Returning a regular DataGroup here may be wrong, since the `bins`
        # property provides a different set of attrs and methods.
        return self._call_method(operator.attrgetter('bins'))

    def copy(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('copy', *args, **kwargs))

    def bin(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('bin', *args, **kwargs))

    def group(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('group', *args, **kwargs))

    def groupby(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('groupby', *args, **kwargs))

    def hist(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('hist', *args, **kwargs))

    def sum(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('sum', *args, **kwargs))

    def mean(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('mean', *args, **kwargs))

    def min(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('min', *args, **kwargs))

    def max(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('max', *args, **kwargs))

    def transform_coords(self, *args, **kwargs):
        return self._call_method(
            operator.methodcaller('transform_coords', *args, **kwargs))

    def to(self, *args, **kwargs):
        return self._call_method(operator.methodcaller('to', *args, **kwargs))

    def plot(self, *args, **kwargs):
        import plopp
        return plopp.plot(self, *args, **kwargs)


def _data_group_binary(func: Callable, dg1: DataGroup, dg2: DataGroup, *args,
                       **kwargs) -> DataGroup:
    return DataGroup({
        key: func(dg1[key], dg2[key], *args, **kwargs)
        for key in dg1.keys() & dg2.keys()
    })


def data_group_nary(func: Callable, *args, **kwargs) -> DataGroup:
    dgs = filter(lambda x: isinstance(x, DataGroup),
                 itertools.chain(args, kwargs.values()))
    keys = functools.reduce(operator.and_, [dg.keys() for dg in dgs])

    def elem(x, key):
        return x[key] if isinstance(x, DataGroup) else x

    return DataGroup({
        key: func(*[elem(x, key) for x in args],
                  **{name: elem(x, key)
                     for name, x in kwargs.items()})
        for key in keys
    })


def _apply_to_items(func: Callable, dgs: Iterable[DataGroup], *args,
                    **kwargs) -> DataGroup:
    keys = functools.reduce(operator.and_, [dg.keys() for dg in dgs])
    return DataGroup(
        {key: func([dg[key] for dg in dgs], *args, **kwargs)
         for key in keys})
