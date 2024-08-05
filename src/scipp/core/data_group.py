# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

import copy
import functools
import itertools
import numbers
import operator
from collections.abc import Callable, Iterable, MutableMapping
from functools import wraps
from typing import (
    TYPE_CHECKING,
    Any,
    NoReturn,
    TypeVar,
    cast,
    overload,
)

import numpy as np

from .. import _binding
from .cpp_classes import (
    DataArray,
    Dataset,
    DimensionError,
    GroupByDataArray,
    GroupByDataset,
    Variable,
)

if TYPE_CHECKING:
    # typing imports data_group.
    # So the following import would create a cycle at runtime.
    from ..typing import ScippIndex


def _item_dims(item):
    return getattr(item, 'dims', ())


def _is_binned(item):
    from .bins import Bins

    if isinstance(item, Bins):
        return True
    return hasattr(item, 'bins') and item.bins is not None


def _summarize(item):
    if isinstance(item, DataGroup):
        return f'{type(item).__name__}({len(item)}, {item.sizes})'
    if hasattr(item, 'sizes'):
        return f'{type(item).__name__}({item.sizes})'
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
    return isinstance(key, list | np.ndarray)


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
        if not all(isinstance(k, str) for k in self._items.keys()):
            raise ValueError("DataGroup keys must be strings.")

    def __copy__(self) -> DataGroup:
        return DataGroup(copy.copy(self._items))

    def __len__(self) -> int:
        """Return the number of items in the data group."""
        return len(self._items)

    def __iter__(self):
        yield from self._items

    @overload
    def __getitem__(self, name: str) -> Any: ...

    @overload
    def __getitem__(self, name: ScippIndex) -> DataGroup: ...

    def __getitem__(self, name):
        """Return item of given name or index all items.

        When ``name`` is a string, return the item of the given name. Otherwise, this
        returns a new DataGroup, with items created by indexing the items in this
        DataGroup. This may perform, e.g., Scipp's positional indexing, label-based
        indexing, or advanced indexing on items that are scipp.Variable or
        scipp.DataArray.

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
                    "an explicit dimension label, e.g., var['x', 0] instead of var[0]."
                )
            dim = self.dims[0]
            index = name
        else:
            dim, index = name
        return DataGroup(
            {
                key: var[dim, index]
                if (isinstance(var, Bins) or dim in _item_dims(var))
                else var
                for key, var in self.items()
            }
        )

    @overload
    def __setitem__(self, name: str, value: Any): ...

    def __setitem__(self, name, value):
        """Set self[key] to value."""
        if isinstance(name, str):
            self._items[name] = value
        else:
            raise TypeError('Keys must be strings')

    def __delitem__(self, name: str):
        """Delete self[key]."""
        del self._items[name]

    def __sizeof__(self) -> int:
        return self.underlying_size()

    def underlying_size(self) -> int:
        # TODO Return the underlying size of all items in DataGroup
        total_size = super.__sizeof__(self)
        for item in self.values():
            if isinstance(item, DataArray | Dataset | Variable | DataGroup):
                total_size += item.underlying_size()
            elif hasattr(item, 'nbytes'):
                total_size += item.nbytes
            else:
                total_size += item.__sizeof__()

        return total_size

    @property
    def dims(self) -> tuple[str, ...]:
        """Union of dims of all items. Non-Scipp items are handled as dims=()."""
        return tuple(self.sizes)

    @property
    def ndim(self):
        """Number of dimensions, i.e., len(self.dims)."""
        return len(self.dims)

    @property
    def shape(self) -> tuple[int | None, ...]:
        """Union of shape of all items. Non-Scipp items are handled as shape=()."""
        return tuple(self.sizes.values())

    @property
    def sizes(self) -> dict[str, int | None]:
        """Dict combining dims and shape, i.e., mapping dim labels to their size."""
        all_sizes = {}
        for x in self.values():
            for dim, size in getattr(x, 'sizes', {}).items():
                all_sizes.setdefault(dim, set()).add(size)
        return {d: next(iter(s)) if len(s) == 1 else None for d, s in all_sizes.items()}

    def _repr_html_(self):
        from ..visualization.formatting_datagroup_html import datagroup_repr

        return datagroup_repr(self)

    def __repr__(self):
        r = f'DataGroup(sizes={self.sizes}, keys=[\n'
        for name, var in self.items():
            r += f'    {name}: {_summarize(var)},\n'
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

    def _transform_dim(
        self, func: Callable, *, dim: None | str | Iterable[str], **kwargs
    ) -> DataGroup:
        """Transform items that depend on one or more dimensions given by `dim`."""
        dims = (dim,) if isinstance(dim, str) else dim

        def intersects(item):
            item_dims = _item_dims(item)
            if dims is None:
                return item_dims != ()
            return set(dims).intersection(item_dims) != set()

        return DataGroup(
            {
                key: v
                if not intersects(v)
                else operator.methodcaller(func, dim, **kwargs)(v)
                for key, v in self.items()
            }
        )

    def _reduce(
        self, method: str, dim: None | str | tuple[str, ...] = None, **kwargs
    ) -> DataGroup:
        reduce_all = operator.methodcaller(method, **kwargs)

        def _reduce_child(v, dim):
            if isinstance(v, GroupByDataArray | GroupByDataset):
                child_dims = (dim,)
            else:
                child_dims = _item_dims(v)
            # Reduction operations on binned data implicitly reduce over bin content.
            # Therefore, a purely dimension-based logic is not sufficient to determine
            # if the item has to be reduced or not.
            binned = _is_binned(v)
            if child_dims == () and not binned:
                return v
            if dim is None:
                return reduce_all(v)
            if isinstance(dim, str):
                dims_to_reduce = dim if dim in child_dims else ()
            else:
                dims_to_reduce = tuple(d for d in dim if d in child_dims)
            if dims_to_reduce == () and binned:
                return reduce_all(v)
            return (
                v
                if dims_to_reduce == ()
                else operator.methodcaller(method, dims_to_reduce, **kwargs)(v)
            )

        return DataGroup({key: _reduce_child(v, dim) for key, v in self.items()})

    def copy(self, deep: bool = True) -> DataGroup:
        return copy.deepcopy(self) if deep else copy.copy(self)

    def all(self, *args, **kwargs):
        return self._reduce('all', *args, **kwargs)

    def any(self, *args, **kwargs):
        return self._reduce('any', *args, **kwargs)

    def astype(self, *args, **kwargs):
        return self.apply(operator.methodcaller('astype', *args, **kwargs))

    def bin(self, *args, **kwargs):
        return self.apply(operator.methodcaller('bin', *args, **kwargs))

    def broadcast(self, *args, **kwargs):
        return self.apply(operator.methodcaller('broadcast', *args, **kwargs))

    def ceil(self, *args, **kwargs):
        return self.apply(operator.methodcaller('ceil', *args, **kwargs))

    def flatten(self, dims: None | Iterable[str] = None, **kwargs):
        return self._transform_dim('flatten', dim=dims, **kwargs)

    def floor(self, *args, **kwargs):
        return self.apply(operator.methodcaller('floor', *args, **kwargs))

    def fold(self, dim: str, **kwargs):
        return self._transform_dim('fold', dim=dim, **kwargs)

    def group(self, *args, **kwargs):
        return self.apply(operator.methodcaller('group', *args, **kwargs))

    def groupby(self, *args, **kwargs):
        return self.apply(operator.methodcaller('groupby', *args, **kwargs))

    def hist(self, *args, **kwargs):
        return self.apply(operator.methodcaller('hist', *args, **kwargs))

    def max(self, *args, **kwargs):
        return self._reduce('max', *args, **kwargs)

    def mean(self, *args, **kwargs):
        return self._reduce('mean', *args, **kwargs)

    def median(self, *args, **kwargs):
        return self._reduce('median', *args, **kwargs)

    def min(self, *args, **kwargs):
        return self._reduce('min', *args, **kwargs)

    def nanhist(self, *args, **kwargs):
        return self.apply(operator.methodcaller('nanhist', *args, **kwargs))

    def nanmax(self, *args, **kwargs):
        return self._reduce('nanmax', *args, **kwargs)

    def nanmean(self, *args, **kwargs):
        return self._reduce('nanmean', *args, **kwargs)

    def nanmedian(self, *args, **kwargs):
        return self._reduce('nanmedian', *args, **kwargs)

    def nanmin(self, *args, **kwargs):
        return self._reduce('nanmin', *args, **kwargs)

    def nansum(self, *args, **kwargs):
        return self._reduce('nansum', *args, **kwargs)

    def nanstd(self, *args, **kwargs):
        return self._reduce('nanstd', *args, **kwargs)

    def nanvar(self, *args, **kwargs):
        return self._reduce('nanvar', *args, **kwargs)

    def rebin(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rebin', *args, **kwargs))

    def rename(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rename', *args, **kwargs))

    def rename_dims(self, *args, **kwargs):
        return self.apply(operator.methodcaller('rename_dims', *args, **kwargs))

    def round(self, *args, **kwargs):
        return self.apply(operator.methodcaller('round', *args, **kwargs))

    def squeeze(self, *args, **kwargs):
        return self._reduce('squeeze', *args, **kwargs)

    def std(self, *args, **kwargs):
        return self._reduce('std', *args, **kwargs)

    def sum(self, *args, **kwargs):
        return self._reduce('sum', *args, **kwargs)

    def to(self, *args, **kwargs):
        return self.apply(operator.methodcaller('to', *args, **kwargs))

    def transform_coords(self, *args, **kwargs):
        return self.apply(operator.methodcaller('transform_coords', *args, **kwargs))

    def transpose(self, dims: None | tuple[str, ...] = None):
        return self._transform_dim('transpose', dim=dims)

    def var(self, *args, **kwargs):
        return self._reduce('var', *args, **kwargs)

    def plot(self, *args, **kwargs):
        import plopp

        return plopp.plot(self, *args, **kwargs)

    def __eq__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise equal."""
        return data_group_nary(operator.eq, self, other)

    def __ne__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise not-equal."""
        return data_group_nary(operator.ne, self, other)

    def __gt__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise greater-than."""
        return data_group_nary(operator.gt, self, other)

    def __ge__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise greater-equal."""
        return data_group_nary(operator.ge, self, other)

    def __lt__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise less-than."""
        return data_group_nary(operator.lt, self, other)

    def __le__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Item-wise less-equal."""
        return data_group_nary(operator.le, self, other)

    def __add__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, self, other)

    def __sub__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, self, other)

    def __mul__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, self, other)

    def __truediv__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, self, other)

    def __floordiv__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, self, other)

    def __mod__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, self, other)

    def __pow__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, self, other)

    def __radd__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, other, self)

    def __rsub__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, other, self)

    def __rmul__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, other, self)

    def __rtruediv__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, other, self)

    def __rfloordiv__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, other, self)

    def __rmod__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, other, self)

    def __rpow__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, other, self)

    def __and__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Return the element-wise ``and`` of items."""
        return data_group_nary(operator.and_, self, other)

    def __or__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Return the element-wise ``or`` of items."""
        return data_group_nary(operator.or_, self, other)

    def __xor__(
        self, other: DataGroup | DataArray | Variable | numbers.Real
    ) -> DataGroup:
        """Return the element-wise ``xor`` of items."""
        return data_group_nary(operator.xor, self, other)

    def __invert__(self) -> DataGroup:
        """Return the element-wise ``or`` of items."""
        return self.apply(operator.invert)


def _data_group_binary(
    func: Callable, dg1: DataGroup, dg2: DataGroup, *args, **kwargs
) -> DataGroup:
    return DataGroup(
        {
            key: func(dg1[key], dg2[key], *args, **kwargs)
            for key in dg1.keys() & dg2.keys()
        }
    )


def data_group_nary(func: Callable, *args, **kwargs) -> DataGroup:
    dgs = filter(
        lambda x: isinstance(x, DataGroup), itertools.chain(args, kwargs.values())
    )
    keys = functools.reduce(operator.and_, [dg.keys() for dg in dgs])

    def elem(x, key):
        return x[key] if isinstance(x, DataGroup) else x

    return DataGroup(
        {
            key: func(
                *[elem(x, key) for x in args],
                **{name: elem(x, key) for name, x in kwargs.items()},
            )
            for key in keys
        }
    )


def _apply_to_items(
    func: Callable, dgs: Iterable[DataGroup], *args, **kwargs
) -> DataGroup:
    keys = functools.reduce(operator.and_, [dg.keys() for dg in dgs])
    return DataGroup(
        {key: func([dg[key] for dg in dgs], *args, **kwargs) for key in keys}
    )


_F = TypeVar('_F', bound=Callable[..., Any])


def data_group_overload(func: _F) -> _F:
    """Add an overload for DataGroup to a function.

    If the first argument of the function is a data group,
    then the decorated function is mapped over all items.
    It is applied recursively for items that are themselves data groups.

    Otherwise, the original function is applied directly.

    Parameters
    ----------
    func:
        Function to decorate.

    Returns
    -------
    :
        Decorated function.
    """

    # Do not assign '__annotations__' because that causes an error in Sphinx.
    @wraps(func, assigned=('__module__', '__name__', '__qualname__', '__doc__'))
    def impl(data, *args, **kwargs):
        if isinstance(data, DataGroup):
            return data.apply(impl, *args, **kwargs)
        return func(data, *args, **kwargs)

    return cast(_F, impl)


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
    def impl(self, other: DataGroup | DataArray | Variable | numbers.Real) -> NoReturn:
        raise TypeError(f'In-place operation i{name} is not supported by DataGroup.')

    return impl


for _name in ('add', 'sub', 'mul', 'truediv', 'floordiv', 'mod', 'pow'):
    full_name = f'__i{_name}__'
    _binding.bind_function_as_method(
        cls=DataGroup, name=full_name, func=_make_inplace_binary_op(full_name)
    )

del _name, full_name
