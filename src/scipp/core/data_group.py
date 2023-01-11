# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

import functools
import operator
from collections.abc import MutableMapping
from typing import Any, Callable, Iterable, overload

from ..typing import ScippIndex
from .cpp_classes import DataArray, Dataset, Variable


def _item_dims(item):
    return getattr(item, 'dims', ())


def _summarize(item):
    if isinstance(item, DataGroup):
        return f'{type(item).__name__}({len(item)}, {item.sizes})\n'
    if hasattr(item, 'sizes'):
        return f'{type(item).__name__}({item.sizes})\n'
    return str(item)


def _is_positional_index(key) -> bool:
    if isinstance(key, int):
        return True
    if isinstance(key, slice):
        return isinstance(key.start, int)
    return False


class DataGroup(MutableMapping):
    """
    A dict-like group of data. Additionally provides dims and shape properties.

    DataGroup acts like a Python dict but additionally supports Scipp functionality
    such as positional- and label-based indexing and Scipp operations by mapping them
    to the values in the dict. This may happen recursively to support tree-like data
    structures.
    """

    def __init__(self, items=None, /):
        self._items = dict(items) if items is not None else {}

    def __len__(self) -> int:
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
        if isinstance(name, str):
            return self._items[name]
        if name == ():
            return DataGroup({key: var[()] for key, var in self.items()})
        dim, index = name
        if _is_positional_index(index) and self.sizes[dim] is None:
            raise ValueError(
                f"Positional indexing dim {dim} not possible as the length is not "
                "unique.")
        return DataGroup({
            key: var[dim, index] if dim in _item_dims(var) else var
            for key, var in self.items()
        })

    @overload
    def __setitem__(self, name: str, value: Any):
        ...

    @overload
    def __setitem__(self, name: ScippIndex, value: DataGroup):
        ...

    def __setitem__(self, name, value):
        self._items[name] = value

    def __delitem__(self, name: str):
        del self._items[name]

    @property
    def dims(self):
        dims = ()
        for var in self.values():
            # Preserve insertion order
            for dim in _item_dims(var):
                if dim not in dims:
                    dims = dims + (dim, )
        return dims

    @property
    def ndim(self):
        return len(self.dims)

    @property
    def shape(self):

        def dim_size(dim):
            sizes = {var.sizes[dim] for var in self.values() if dim in _item_dims(var)}
            if len(sizes) == 1:
                return next(iter(sizes))
            return None

        return tuple(dim_size(dim) for dim in self.dims)

    @property
    def sizes(self):
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


def _apply_to_items(func: Callable, dgs: Iterable[DataGroup], *args,
                    **kwargs) -> DataGroup:
    keys = functools.reduce(operator.and_, [dg.keys() for dg in dgs])
    return DataGroup(
        {key: func([dg[key] for dg in dgs], *args, **kwargs)
         for key in keys})
