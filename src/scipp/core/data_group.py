# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

import copy
import functools
import itertools
import numbers
import operator
from collections.abc import (
    Callable,
    ItemsView,
    Iterable,
    Iterator,
    KeysView,
    Mapping,
    MutableMapping,
    Sequence,
    ValuesView,
)
from functools import wraps
from typing import (
    TYPE_CHECKING,
    Any,
    Concatenate,
    NoReturn,
    ParamSpec,
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
    Unit,
    Variable,
)

if TYPE_CHECKING:
    # Avoid cyclic imports
    from ..coords.graph import GraphDict
    from ..typing import ScippIndex
    from .bins import Bins

_T = TypeVar("_T")  # Any type
_V = TypeVar("_V")  # Value type of self
_R = TypeVar("_R")  # Return type of a callable
_P = ParamSpec('_P')


def _item_dims(item: Any) -> tuple[str, ...]:
    return getattr(item, 'dims', ())


def _is_binned(item: Any) -> bool:
    from .bins import Bins

    if isinstance(item, Bins):
        return True
    return getattr(item, 'bins', None) is not None


def _summarize(item: Any) -> str:
    if isinstance(item, DataGroup):
        return f'{type(item).__name__}({len(item)}, {item.sizes})'
    if hasattr(item, 'sizes'):
        return f'{type(item).__name__}({item.sizes})'
    return str(item)


def _is_positional_index(key: Any) -> bool:
    def is_int(x: object) -> bool:
        return isinstance(x, numbers.Integral)

    if is_int(key):
        return True
    if isinstance(key, slice):
        if is_int(key.start) or is_int(key.stop) or is_int(key.step):
            return True
        if key.start is None and key.stop is None and key.step is None:
            return True
    return False


def _is_list_index(key: Any) -> bool:
    return isinstance(key, list | np.ndarray)


class DataGroup(MutableMapping[str, _V]):
    """
    A dict-like group of data. Additionally provides dims and shape properties.

    DataGroup acts like a Python dict but additionally supports Scipp functionality
    such as positional- and label-based indexing and Scipp operations by mapping them
    to the values in the dict. This may happen recursively to support tree-like data
    structures.

    .. versionadded:: 23.01.0
    """

    def __init__(
        self, /, *args: Iterable[tuple[str, _V]] | Mapping[str, _V], **kwargs: _V
    ) -> None:
        self._items = dict(*args, **kwargs)
        if not all(isinstance(k, str) for k in self._items.keys()):
            raise ValueError("DataGroup keys must be strings.")

    def __copy__(self) -> DataGroup[_V]:
        return DataGroup(copy.copy(self._items))

    def __len__(self) -> int:
        """Return the number of items in the data group."""
        return len(self._items)

    def __iter__(self) -> Iterator[str]:
        return iter(self._items)

    def keys(self) -> KeysView[str]:
        return self._items.keys()

    def values(self) -> ValuesView[_V]:
        return self._items.values()

    def items(self) -> ItemsView[str, _V]:
        return self._items.items()

    @overload
    def __getitem__(self, name: str) -> _V: ...

    @overload
    def __getitem__(self, name: ScippIndex) -> DataGroup[_V]: ...

    def __getitem__(self, name: Any) -> Any:
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
            return cast(DataGroup[Any], self).apply(operator.itemgetter(name))
        if isinstance(name, Variable):  # boolean indexing
            return cast(DataGroup[Any], self).apply(operator.itemgetter(name))
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
                key: var[dim, index]  # type: ignore[index]
                if (isinstance(var, Bins) or dim in _item_dims(var))
                else var
                for key, var in self.items()
            }
        )

    def __setitem__(self, name: str, value: _V) -> None:
        """Set self[key] to value."""
        if isinstance(name, str):
            self._items[name] = value
        else:
            raise TypeError('Keys must be strings')

    def __delitem__(self, name: str) -> None:
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
    def ndim(self) -> int:
        """Number of dimensions, i.e., len(self.dims)."""
        return len(self.dims)

    @property
    def shape(self) -> tuple[int | None, ...]:
        """Union of shape of all items. Non-Scipp items are handled as shape=()."""
        return tuple(self.sizes.values())

    @property
    def sizes(self) -> dict[str, int | None]:
        """Dict combining dims and shape, i.e., mapping dim labels to their size."""
        all_sizes: dict[str, set[int]] = {}
        for x in self.values():
            for dim, size in getattr(x, 'sizes', {}).items():
                all_sizes.setdefault(dim, set()).add(size)
        return {d: next(iter(s)) if len(s) == 1 else None for d, s in all_sizes.items()}

    def _repr_html_(self) -> str:
        from ..visualization.formatting_datagroup_html import datagroup_repr

        return datagroup_repr(self)

    def __repr__(self) -> str:
        r = f'DataGroup(sizes={self.sizes}, keys=[\n'
        for name, var in self.items():
            r += f'    {name}: {_summarize(var)},\n'
        r += '])'
        return r

    def __str__(self) -> str:
        return f'DataGroup(sizes={self.sizes}, keys={list(self.keys())})'

    @property
    def bins(self) -> DataGroup[DataGroup[Any] | Bins[Any] | None]:
        # TODO Returning a regular DataGroup here may be wrong, since the `bins`
        # property provides a different set of attrs and methods.
        return self.apply(operator.attrgetter('bins'))

    def apply(
        self,
        func: Callable[Concatenate[_V, _P], _R],
        *args: _P.args,
        **kwargs: _P.kwargs,
    ) -> DataGroup[_R]:
        """Call func on all values and return new DataGroup containing the results."""
        return DataGroup({key: func(v, *args, **kwargs) for key, v in self.items()})

    def _transform_dim(
        self, func: str, *, dim: None | str | Iterable[str], **kwargs: Any
    ) -> DataGroup[Any]:
        """Transform items that depend on one or more dimensions given by `dim`."""
        dims = (dim,) if isinstance(dim, str) else dim

        def intersects(item: _V) -> bool:
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
        self, method: str, dim: None | str | Sequence[str] = None, **kwargs: Any
    ) -> DataGroup[Any]:
        reduce_all = operator.methodcaller(method, **kwargs)

        def _reduce_child(v: _V) -> Any:
            if isinstance(v, GroupByDataArray | GroupByDataset):
                child_dims: tuple[None | str | Sequence[str], ...] = (dim,)
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
                dims_to_reduce: tuple[str, ...] | str = dim if dim in child_dims else ()
            else:
                dims_to_reduce = tuple(d for d in dim if d in child_dims)
            if dims_to_reduce == () and binned:
                return reduce_all(v)
            return (
                v
                if dims_to_reduce == ()
                else operator.methodcaller(method, dims_to_reduce, **kwargs)(v)
            )

        return DataGroup({key: _reduce_child(v) for key, v in self.items()})

    def copy(self, deep: bool = True) -> DataGroup[_V]:
        return copy.deepcopy(self) if deep else copy.copy(self)

    def all(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('all', dim)

    def any(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('any', dim)

    def astype(self, type: Any, *, copy: bool = True) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('astype', type, copy=copy))

    def bin(
        self,
        arg_dict: Mapping[str, int | Variable] | None = None,
        /,
        **kwargs: int | Variable,
    ) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('bin', arg_dict, **kwargs))

    @overload
    def broadcast(
        self,
        *,
        dims: Sequence[str],
        shape: Sequence[int],
    ) -> DataGroup[_V]: ...

    @overload
    def broadcast(
        self,
        *,
        sizes: dict[str, int],
    ) -> DataGroup[_V]: ...

    def broadcast(
        self,
        *,
        dims: Sequence[str] | None = None,
        shape: Sequence[int] | None = None,
        sizes: dict[str, int] | None = None,
    ) -> DataGroup[_V]:
        return self.apply(
            operator.methodcaller('broadcast', dims=dims, shape=shape, sizes=sizes)
        )

    def ceil(self) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('ceil'))

    def flatten(
        self, dims: Sequence[str] | None = None, to: str | None = None
    ) -> DataGroup[_V]:
        return self._transform_dim('flatten', dim=dims, to=to)

    def floor(self) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('floor'))

    @overload
    def fold(
        self,
        dim: str,
        *,
        dims: Sequence[str],
        shape: Sequence[int],
    ) -> DataGroup[_V]: ...

    @overload
    def fold(
        self,
        dim: str,
        *,
        sizes: dict[str, int],
    ) -> DataGroup[_V]: ...

    def fold(
        self,
        dim: str,
        *,
        dims: Sequence[str] | None = None,
        shape: Sequence[int] | None = None,
        sizes: dict[str, int] | None = None,
    ) -> DataGroup[_V]:
        return self._transform_dim('fold', dim=dim, dims=dims, shape=shape, sizes=sizes)

    def group(self, /, *args: str | Variable) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('group', *args))

    def groupby(
        self, /, group: Variable | str, *, bins: Variable | None = None
    ) -> DataGroup[GroupByDataArray | GroupByDataset]:
        return self.apply(operator.methodcaller('groupby', group, bins=bins))

    def hist(
        self,
        arg_dict: dict[str, int | Variable] | None = None,
        /,
        **kwargs: int | Variable,
    ) -> DataGroup[DataArray | Dataset]:
        return self.apply(operator.methodcaller('hist', arg_dict, **kwargs))

    def max(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('max', dim)

    def mean(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('mean', dim)

    def median(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('median', dim)

    def min(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('min', dim)

    def nanhist(
        self,
        arg_dict: dict[str, int | Variable] | None = None,
        /,
        **kwargs: int | Variable,
    ) -> DataGroup[DataArray]:
        return self.apply(operator.methodcaller('nanhist', arg_dict, **kwargs))

    def nanmax(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('nanmax', dim)

    def nanmean(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('nanmean', dim)

    def nanmedian(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('nanmedian', dim)

    def nanmin(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('nanmin', dim)

    def nansum(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('nansum', dim)

    def nanstd(
        self, dim: None | str | tuple[str, ...] = None, *, ddof: int
    ) -> DataGroup[_V]:
        return self._reduce('nanstd', dim, ddof=ddof)

    def nanvar(
        self, dim: None | str | tuple[str, ...] = None, *, ddof: int
    ) -> DataGroup[_V]:
        return self._reduce('nanvar', dim, ddof=ddof)

    def rebin(
        self,
        arg_dict: dict[str, int | Variable] | None = None,
        /,
        **kwargs: int | Variable,
    ) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('rebin', arg_dict, **kwargs))

    def rename(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('rename', dims_dict, **names))

    def rename_dims(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('rename_dims', dims_dict, **names))

    def round(self, *, decimals: int = 0) -> DataGroup[_V]:
        return self.apply(operator.methodcaller('round', decimals=decimals))

    def squeeze(self, dim: str | Sequence[str] | None = None) -> DataGroup[_V]:
        return self._reduce('squeeze', dim)

    def std(
        self, dim: None | str | tuple[str, ...] = None, *, ddof: int
    ) -> DataGroup[_V]:
        return self._reduce('std', dim, ddof=ddof)

    def sum(self, dim: None | str | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._reduce('sum', dim)

    def to(
        self,
        *,
        unit: Unit | str | None = None,
        dtype: Any | None = None,
        copy: bool = True,
    ) -> DataGroup[_V]:
        return self.apply(
            operator.methodcaller('to', unit=unit, dtype=dtype, copy=copy)
        )

    def transform_coords(
        self,
        targets: str | Iterable[str] | None = None,
        /,
        graph: GraphDict | None = None,
        *,
        rename_dims: bool = True,
        keep_aliases: bool = True,
        keep_intermediate: bool = True,
        keep_inputs: bool = True,
        quiet: bool = False,
        **kwargs: Callable[..., Variable],
    ) -> DataGroup[_V]:
        return self.apply(
            operator.methodcaller(
                'transform_coords',
                targets,
                graph=graph,
                rename_dims=rename_dims,
                keep_aliases=keep_aliases,
                keep_intermediate=keep_intermediate,
                keep_inputs=keep_inputs,
                quiet=quiet,
                **kwargs,
            )
        )

    def transpose(self, dims: None | tuple[str, ...] = None) -> DataGroup[_V]:
        return self._transform_dim('transpose', dim=dims)

    def var(
        self, dim: None | str | tuple[str, ...] = None, *, ddof: int
    ) -> DataGroup[_V]:
        return self._reduce('var', dim, ddof=ddof)

    def plot(self, *args: Any, **kwargs: Any) -> Any:
        import plopp

        return plopp.plot(self, *args, **kwargs)

    def __eq__(  # type: ignore[override]
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise equal."""
        return data_group_nary(operator.eq, self, other)

    def __ne__(  # type: ignore[override]
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise not-equal."""
        return data_group_nary(operator.ne, self, other)

    def __gt__(
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise greater-than."""
        return data_group_nary(operator.gt, self, other)

    def __ge__(
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise greater-equal."""
        return data_group_nary(operator.ge, self, other)

    def __lt__(
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise less-than."""
        return data_group_nary(operator.lt, self, other)

    def __le__(
        self, other: DataGroup[object] | DataArray | Variable | float
    ) -> DataGroup[_V | bool]:
        """Item-wise less-equal."""
        return data_group_nary(operator.le, self, other)

    def __add__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, self, other)

    def __sub__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, self, other)

    def __mul__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, self, other)

    def __truediv__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, self, other)

    def __floordiv__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, self, other)

    def __mod__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, self, other)

    def __pow__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, self, other)

    def __radd__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``add`` item-by-item."""
        return data_group_nary(operator.add, other, self)

    def __rsub__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``sub`` item-by-item."""
        return data_group_nary(operator.sub, other, self)

    def __rmul__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``mul`` item-by-item."""
        return data_group_nary(operator.mul, other, self)

    def __rtruediv__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``truediv`` item-by-item."""
        return data_group_nary(operator.truediv, other, self)

    def __rfloordiv__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``floordiv`` item-by-item."""
        return data_group_nary(operator.floordiv, other, self)

    def __rmod__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``mod`` item-by-item."""
        return data_group_nary(operator.mod, other, self)

    def __rpow__(self, other: DataArray | Variable | float) -> DataGroup[Any]:
        """Apply ``pow`` item-by-item."""
        return data_group_nary(operator.pow, other, self)

    def __and__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Return the element-wise ``and`` of items."""
        return data_group_nary(operator.and_, self, other)

    def __or__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Return the element-wise ``or`` of items."""
        return data_group_nary(operator.or_, self, other)

    def __xor__(
        self, other: DataGroup[Any] | DataArray | Variable | float
    ) -> DataGroup[Any]:
        """Return the element-wise ``xor`` of items."""
        return data_group_nary(operator.xor, self, other)

    def __invert__(self) -> DataGroup[Any]:
        """Return the element-wise ``or`` of items."""
        return self.apply(operator.invert)  # type: ignore[arg-type]

    def save_hdf5(self, filename: object) -> None:
        # Stub for type checking, overridden at runtime in __init__.py
        pass


def data_group_nary(
    func: Callable[..., _R], *args: Any, **kwargs: Any
) -> DataGroup[_R]:
    dgs = [
        arg
        for arg in itertools.chain(args, kwargs.values())
        if isinstance(arg, DataGroup)
    ]
    keys = _key_intersection(dgs)

    def elem(x: Any, key: str) -> Any:
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


def apply_to_items(
    func: Callable[..., _R], dgs: Iterable[DataGroup[Any]], *args: Any, **kwargs: Any
) -> DataGroup[_R]:
    keys = _key_intersection(list(dgs))
    return DataGroup(
        {key: func([dg[key] for dg in dgs], *args, **kwargs) for key in keys}
    )


def _key_intersection(dgs: list[DataGroup[Any]]) -> list[str]:
    return [
        key
        # Use the keys in the order of the first argument
        for key in dgs[0].keys()
        # if the key is in all DataGroups
        if key in functools.reduce(operator.and_, (dg.keys() for dg in dgs))
    ]


def data_group_overload(
    func: Callable[Concatenate[_T, _P], _R],
) -> Callable[..., _R | DataGroup[_R]]:
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
    def impl(
        data: _T | DataGroup[Any], *args: _P.args, **kwargs: _P.kwargs
    ) -> _R | DataGroup[_R]:
        if isinstance(data, DataGroup):
            return data.apply(impl, *args, **kwargs)  # type: ignore[arg-type]
        return func(data, *args, **kwargs)

    return impl


# There are currently no in-place operations (__iadd__, etc.) because they require
# a check if the operation would fail before doing it. As otherwise, a failure could
# leave a partially modified data group behind. Dataset implements such a check, but
# it is simpler than for DataGroup because the latter supports more data types.
# So for now, we went with the simple solution and
# not support in-place operations at all.
#
# Binding these functions dynamically has the added benefit that type checkers think
# that the operations are not implemented.
def _make_inplace_binary_op(name: str) -> Callable[..., NoReturn]:
    def impl(
        self: DataGroup[Any], other: DataGroup[Any] | DataArray | Variable | float
    ) -> NoReturn:
        raise TypeError(f'In-place operation i{name} is not supported by DataGroup.')

    return impl


for _name in ('add', 'sub', 'mul', 'truediv', 'floordiv', 'mod', 'pow'):
    full_name = f'__i{_name}__'
    _binding.bind_function_as_method(
        cls=DataGroup, name=full_name, func=_make_inplace_binary_op(full_name)
    )

del _name, full_name
