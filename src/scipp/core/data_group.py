# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

import copy
import functools
import itertools
import numbers
import operator
from collections.abc import MutableMapping
from typing import Any, Callable, Iterable, NoReturn, Union, overload

import numpy as np

from .. import _binding
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


def _is_list_index(key) -> bool:
    return isinstance(key, (list, np.ndarray))


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

    def __copy__(self) -> DataGroup:
        return DataGroup(copy.copy(self._items))

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
        DataGroup. This may perform, e.g., Scipp's positional indexing, label-based
        indexing, or advanced indexing on items that are scipp.Variable or
        scipp.DataArray.

        Positional indexing is only possible when the shape of all items is consistent
        for the indexed dimension.

        Label-based indexing is only possible when all items have a coordinate for the
        indexed dimension.

        Advanced indexing comprises integer-array indexing and boolean-variable
        indexing. Unlike positional indexing, integer-array indexing works even when
        the item shapes are inconsistent for the indexed dimensions, provided that all
        items contain the maximal index in the integer array. Boolean-variable indexing
        is only possible when the shape of all items is compatible with the boolean
        variable.
        """
        from .bins import Bins
        if isinstance(name, str):
            return self._items[name]
        if isinstance(name, tuple) and name == ():
            return self.apply(operator.itemgetter(name))
        if isinstance(name, Variable):  # boolean indexing
            return self.apply(operator.itemgetter(name))
        if _is_positional_index(name) or _is_list_index(name):
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
            key: var[dim, index] if
            (isinstance(var, Bins) or dim in _item_dims(var)) else var
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
        itemsizes = [getattr(x, 'sizes', {}) for x in self.values()]

        def dim_size(dim):
            sizes = {sizes[dim] for sizes in itemsizes if dim in sizes}
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
            out += f"<details style=\"padding-left:2em\"><summary>" \
                   f"{name}: {_summarize(item)}</summary>{html}</details>"
        return out

    def _repr_html_(self):
        out = ''
        out += f"<details open=\"open\"><summary>DataGroup" \
               f"({len(self)})</summary>"
        out += self._make_html()
        out += "</details>"
        return out

    def __repr__(self):
        r = f'DataGroup(sizes={self.sizes}, keys=[\n'
        for name, var in self.items():
            r += f'    {name}: {_summarize(var)[:-1]},\n'
        r += '])'
        return r

    def __str__(self):
        return f'DataGroup(sizes={self.sizes}, keys={list(self.keys())})'

    @property
    def bins(self):
        # TODO Returning a regular DataGroup here may be wrong, since the `bins`
        # property provides a different set of attrs and methods.
        return self.apply(operator.attrgetter('bins'))

    def apply(self, func: Callable, *args, **kwargs) -> DataGroup:
        """Call func on all values and return new DataGroup containing the results."""
        return DataGroup({key: func(v, *args, **kwargs) for key, v in self.items()})

    def copy(self, deep: bool = True) -> DataGroup:
        return copy.deepcopy(self) if deep else copy.copy(self)

    def all(self, *args, **kwargs):
        return self.apply(operator.methodcaller('all', *args, **kwargs))

    def any(self, *args, **kwargs):
        return self.apply(operator.methodcaller('any', *args, **kwargs))

    def astype(self, *args, **kwargs):
        return self.apply(operator.methodcaller('astype', *args, **kwargs))

    def bin(self, *args, **kwargs):
        return self.apply(operator.methodcaller('bin', *args, **kwargs))

    def broadcast(self, *args, **kwargs):
        return self.apply(operator.methodcaller('broadcast', *args, **kwargs))

    def ceil(self, *args, **kwargs):
        return self.apply(operator.methodcaller('ceil', *args, **kwargs))

    def flatten(self, *args, **kwargs):
        return self.apply(operator.methodcaller('flatten', *args, **kwargs))

    def floor(self, *args, **kwargs):
        return self.apply(operator.methodcaller('floor', *args, **kwargs))

    def fold(self, *args, **kwargs):
        return self.apply(operator.methodcaller('fold', *args, **kwargs))

    def group(self, *args, **kwargs):
        return self.apply(operator.methodcaller('group', *args, **kwargs))

    def groupby(self, *args, **kwargs):
        return self.apply(operator.methodcaller('groupby', *args, **kwargs))

    def hist(self, *args, **kwargs):
        return self.apply(operator.methodcaller('hist', *args, **kwargs))

    def max(self, *args, **kwargs):
        return self.apply(operator.methodcaller('max', *args, **kwargs))

    def mean(self, *args, **kwargs):
        return self.apply(operator.methodcaller('mean', *args, **kwargs))

    def min(self, *args, **kwargs):
        return self.apply(operator.methodcaller('min', *args, **kwargs))

    def nanhist(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nanhist', *args, **kwargs))

    def nanmax(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nanmax', *args, **kwargs))

    def nanmean(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nanmean', *args, **kwargs))

    def nanmin(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nanmin', *args, **kwargs))

    def nansum(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nansum', *args, **kwargs))

    def rebin(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rebin', *args, **kwargs))

    def rename(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rename', *args, **kwargs))

    def rename_dims(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rename_dims', *args, **kwargs))

    def round(self, *args, **kwargs):
        return self.apply(operator.methodcaller('round', *args, **kwargs))

    def squeeze(self, *args, **kwargs):
        return self.apply(operator.methodcaller('squeeze', *args, **kwargs))

    def sum(self, *args, **kwargs):
        return self.apply(operator.methodcaller('sum', *args, **kwargs))

    def to(self, *args, **kwargs):
        return self.apply(operator.methodcaller('to', *args, **kwargs))

    def transform_coords(self, *args, **kwargs):
        return self.apply(operator.methodcaller('transform_coords', *args, **kwargs))

    def transpose(self, *args, **kwargs):
        return self.apply(operator.methodcaller('transpose', *args, **kwargs))

    def plot(self, *args, **kwargs):
        import plopp
        return plopp.plot(self, *args, **kwargs)

    def __eq__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise equal."""
        return data_group_nary(operator.eq, self, other)

    def __ne__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise not-equal."""
        return data_group_nary(operator.ne, self, other)

    def __gt__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise greater-than."""
        return data_group_nary(operator.gt, self, other)

    def __ge__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise greater-equal."""
        return data_group_nary(operator.ge, self, other)

    def __lt__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise less-than."""
        return data_group_nary(operator.lt, self, other)

    def __le__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Item-wise less-equal."""
        return data_group_nary(operator.le, self, other)

    def __add__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, self, other)

    def __sub__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, self, other)

    def __mul__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, self, other)

    def __truediv__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, self, other)

    def __floordiv__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, self, other)

    def __mod__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, self, other)

    def __pow__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, self, other)

    def __radd__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, other, self)

    def __rsub__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, other, self)

    def __rmul__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, other, self)

    def __rtruediv__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, other, self)

    def __rfloordiv__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, other, self)

    def __rmod__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, other, self)

    def __rpow__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, other, self)

    def __and__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Return the element-wise ``and`` of items."""
        return data_group_nary(operator.and_, self, other)

    def __or__(self, other: Union[DataGroup, DataArray, Variable,
                                  numbers.Real]) -> DataGroup:
        """Return the element-wise ``or`` of items."""
        return data_group_nary(operator.or_, self, other)

    def __xor__(
            self, other: Union[DataGroup, DataArray, Variable,
                               numbers.Real]) -> DataGroup:
        """Return the element-wise ``xor`` of items."""
        return data_group_nary(operator.xor, self, other)

    def __invert__(self) -> DataGroup:
        """Return the element-wise ``or`` of items."""
        return self.apply(operator.invert)


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


# There are currently no in-place operations (__iadd__, etc.) because they require
# a check if the operation would fail before doing it. As otherwise, a failure could
# leave a partially modified data group behind. Dataset implements such a check, but
# it is simpler than for DataGroup because the latter supports more data types.
# So for now, we went with the simple solution and
# not support in-place operations at all.
#
# Binding these functions dynamically has the added benefit that type checkers think
# that the operations are not implemented.
def _make_inplace_binary_op(name: str):

    def impl(self, other: Union[DataGroup, DataArray, Variable,
                                numbers.Real]) -> NoReturn:
        raise TypeError(f'In-place operation i{name} is not supported by DataGroup.')

    return impl


for _name in ('add', 'sub', 'mul', 'truediv', 'floordiv', 'mod', 'pow'):
    full_name = f'__i{_name}__'
    _binding.bind_function_as_method(cls=DataGroup,
                                     name=full_name,
                                     func=_make_inplace_binary_op(full_name))

del _name, full_name
