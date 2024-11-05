# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import Callable
from typing import Literal, Sequence, TypedDict

from .._scipp import core as _cpp
from ..typing import Dims, MetaDataMap, VariableLike
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .bin_remapping import concat_bins
from .cpp_classes import DataArray, Dataset, DType, Unit, Variable
from .data_group import DataGroup
from .deprecation import _warn_attr_removal
from .domains import merge_equal_adjacent
from .math import midpoints
from .operations import islinspace
from .shape import concat
from .variable import scalar


class Lookup:
    """Lookup table.

    This is class should never be instantiated manually.
    Instead, use :func:`scipp.lookup`.

    See :func:`scipp.lookup` also for usage examples.
    """

    def __init__(
        self,
        op: Callable,
        func: DataArray,
        dim: str,
        fill_value: Variable | None = None,
    ):
        if (
            not func.masks
            and func.ndim == 1
            and len(func) > 0
            and func.dtype in [DType.bool, DType.int32, DType.int64]
        ):
            # Significant speedup if `func` is large but mostly constant.
            if op == _cpp.buckets.map:
                if not islinspace(func.coords[dim], dim).value:
                    func = merge_equal_adjacent(func)
            else:
                # In this case the C++ implementation currently used no linspace
                # optimization, so the extra check is skipped.
                transition = func.data[:-1] != func.data[1:]
                func = concat([func[0], func[1:][transition]], dim)
        self.op = op
        self.func = func
        self.dim = dim
        self.fill_value = fill_value
        self.__transform_coords_input_keys__ = (dim,)  # for transform_coords

    def __call__(self, var: Variable) -> Variable:
        """Return table values for the given points."""
        return self.op(self.func, var, self.dim, self.fill_value)

    def __getitem__(self, var: Variable) -> Variable:
        """Return table values for the given points."""
        return self(var)


def lookup(
    func: DataArray,
    dim: str | None = None,
    *,
    mode: Literal['previous', 'nearest'] | None = None,
    fill_value: Variable | None = None,
) -> Lookup:
    """Create a "lookup table" from a histogram (data array with bin-edge coord).

    The lookup table can be used to map, e.g., time-stamps to corresponding values
    given by a time-series log.

    Parameters
    ----------
    func:
        Data array defining the lookup table.
    dim:
        Dimension along which the lookup occurs.
    mode:
        Mode used for looking up function values. Must be ``None`` when ``func`` is a
        histogram. Otherwise this defaults to 'nearest'.
    fill_value:
        Value to use for points outside the range of the function as well as points in
        masked regions of the function. If set to None (the default) this will use NaN
        for floating point types and 0 for integral types. Must have the same dtype and
        unit as the function values.

    Returns
    -------
    :
        The created lookup table.

    Examples
    --------

      >>> x = sc.linspace(dim='x', start=0.0, stop=1.0, num=4)
      >>> vals = sc.array(dims=['x'], values=[3, 2, 1])
      >>> hist = sc.DataArray(data=vals, coords={'x': x})
      >>> sc.lookup(hist, 'x')[sc.array(dims=['event'], values=[0.1,0.4,0.1,0.6,0.9])]
      <scipp.Variable> (event: 5)      int64  [dimensionless]  [3, 2, ..., 2, 1]
    """
    if dim is None:
        dim = func.dim
    func = DataArray(func.data, coords={dim: func.coords[dim]}, masks=func.masks)
    if func.dims[-1] != dim:
        # We automatically transpose the data so that `dim` is the inner dimension to
        # ensure contiguous memory access.
        dims = (*[d for d in func.dims if d != dim], dim)
        func.data = func.data.transpose(dims).copy()
        func.coords[dim] = func.coords[dim].transpose(dims).copy()
        for key, mask in func.masks.items():
            func.masks[key] = mask.transpose(
                # Masks potentially have fewer dims than the data.
                [d for d in dims if d in mask.dims] or None
            ).copy()
    if func.coords.is_edges(dim, dim):
        if mode is not None:
            raise ValueError("Input is a histogram, 'mode' must not be set.")
        return Lookup(_cpp.buckets.map, func, dim, fill_value)
    if mode is None:
        mode = 'nearest'
    elif mode not in ['previous', 'nearest']:
        raise ValueError(f"Mode must be one of ['previous', 'nearest'], got '{mode}'")
    if mode == 'nearest' and func.sizes[dim] != 0:
        coord = func.coords[dim]
        lowest = coord[dim, 0:0].max()  # trick to get lowest representable value
        parts = [lowest] if coord.sizes[dim] < 2 else [lowest, midpoints(coord, dim)]
        func.coords[dim] = concat(parts, dim)
    return Lookup(_cpp.lookup_previous, func, dim, fill_value)


class Constituents(TypedDict):
    """A dict with bin constituents."""

    data: Variable | DataArray | Dataset
    """Data content."""
    begin: Variable
    """Begin indices for each bin."""
    end: Variable
    """End indices for each bin."""
    dim: str
    """Dimension in 'data' that the binning applies to."""


class Bins:
    """Proxy for access to bin contents and operations on bins of a variable.

    This class is returned from the `bins` property of variables and should
    generally not be created directly.
    """

    def __init__(self, obj):
        self._obj = obj

    def _data(self):
        try:
            return self._obj.data
        except AttributeError:
            return self._obj

    def __mul__(self, lut: lookup):
        target_dtype = (
            scalar(1, dtype=self.dtype) * scalar(1, dtype=lut.func.dtype)
        ).dtype
        copy = self._obj.to(dtype=target_dtype)
        _cpp.buckets.scale(copy, lut.func, lut.dim)
        return copy

    def __truediv__(self, lut: lookup):
        target_dtype = (
            scalar(1, dtype=self.dtype) / scalar(1, dtype=lut.func.dtype)
        ).dtype
        copy = self._obj.to(dtype=target_dtype)
        _cpp.buckets.scale(copy, _cpp.reciprocal(lut.func), lut.dim)
        return copy

    def __imul__(self, lut: lookup):
        _cpp.buckets.scale(self._obj, lut.func, lut.dim)
        return self

    def __itruediv__(self, lut: lookup):
        _cpp.buckets.scale(self._obj, _cpp.reciprocal(lut.func), lut.dim)
        return self

    def __getitem__(self, key: tuple[str, Variable | slice]):
        """
        Extract events from bins based on labels or label ranges and return a copy.

        This is similar to regular label-based indexing, but considers the event-coords,
        i.e., the coord values of individual bin entries. Unlike normal label-based
        indexing this returns a copy, as a subset of events is extracted.
        """
        dim, index = key
        if isinstance(index, Variable):
            if index.ndim == 0:
                return self._obj.group(index.flatten(to=dim)).squeeze(dim)
        elif isinstance(index, slice):
            from .binning import _upper_bound

            if index.step is not None:
                raise ValueError(
                    "Label-based indexing with step (stride) is not "
                    f"supported. Got '{key}'"
                )
            start = index.start
            stop = index.stop
            if start is None:
                start = self._obj.bins.coords[dim].min()
            if stop is None:
                stop = _upper_bound(self._obj.bins.coords[dim].max())

            if not (isinstance(start, Variable) and isinstance(stop, Variable)):
                raise ValueError(
                    "Bins can only by sliced using label-based indexing. Expected "
                    f"start and stop to be scipp.Variable, got '{start}' and '{stop}'."
                )

            if start > stop:
                if index.start is None:
                    start = stop
                elif index.stop is None:
                    stop = start

            return self._obj.bin({dim: concat([start, stop], dim)}).squeeze(dim)
        raise ValueError(
            f"Unsupported key '{key}'. Expected a dimension label and "
            "a 0-D variable or a dimension label and a slice object with start "
            "and stop given by a 0-D variable."
        )

    @property
    def coords(self) -> MetaDataMap:
        """Coords of the bins"""
        return _cpp._bins_view(self._data()).coords

    def drop_coords(self, coords: str | Sequence[str]) -> DataArray:
        """Drop coords from bin content"""
        content = self.constituents
        content['data'] = content['data'].drop_coords(coords)
        out = self._obj.copy(deep=False)
        out.data = _cpp._bins_no_validate(**content)
        return out

    @property
    def meta(self) -> MetaDataMap:
        """Coords and attrs of the bins

        .. deprecated:: 23.9.0
           Use :py:attr:`coords` with unset alignment flag instead, or
           store attributes in higher-level data structures.
        """
        _warn_attr_removal()
        return self.deprecated_meta

    @property
    def attrs(self) -> MetaDataMap:
        """Attrs of the bins

        .. deprecated:: 23.9.0
           Use :py:attr:`coords` with unset alignment flag instead, or
           store attributes in higher-level data structures.
        """
        _warn_attr_removal()
        return self.deprecated_attrs

    @property
    def deprecated_meta(self) -> MetaDataMap:
        return _cpp._bins_view(self._data()).deprecated_meta

    @property
    def deprecated_attrs(self) -> MetaDataMap:
        return _cpp._bins_view(self._data()).deprecated_attrs

    @property
    def masks(self) -> MetaDataMap:
        """Masks of the bins"""
        return _cpp._bins_view(self._data()).masks

    def drop_masks(self, masks: str | Sequence[str]) -> DataArray:
        """Drop masks from bin content"""
        content = self.constituents
        content['data'] = content['data'].drop_masks(masks)
        out = self._obj.copy(deep=False)
        out.data = _cpp._bins_no_validate(**content)
        return out

    @property
    def data(self) -> Variable:
        """Data of the bins"""
        return _cpp._bins_view(self._data()).data

    @data.setter
    def data(self, data: Variable):
        """Set data of the bins"""
        _cpp._bins_view(self._data()).data = data

    @property
    def unit(self) -> Unit:
        """Unit of the bin elements"""
        return self.constituents['data'].unit

    @unit.setter
    def unit(self, unit: Unit | str):
        """Set unit of the bin elements"""
        self.constituents['data'].unit = unit

    @property
    def dtype(self) -> DType:
        """Data type of the bin elements."""
        return self.constituents['data'].dtype

    @property
    def aligned(self) -> bool:
        """Alignment flag for coordinates of bin elements."""
        return self.constituents['data'].aligned

    @property
    def constituents(self) -> Constituents:
        """Constituents of binned data, as supported by :py:func:`sc.bins`."""
        return _call_cpp_func(_cpp.bins_constituents, self._data())

    def sum(self) -> Variable | DataArray:
        """Sum of events in each bin.

        Returns
        -------
        :
            The sum of each of the input bins.

        See Also
        --------
        scipp.sum:
            For summing non-bin data or summing bins.
        """
        return _call_cpp_func(_cpp.bins_sum, self._obj)

    def nansum(self) -> Variable | DataArray:
        """Sum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The sum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nansum:
            For summing non-bin data or summing bins.
        """
        return _call_cpp_func(_cpp.bins_nansum, self._obj)

    def mean(self) -> Variable | DataArray:
        """Arithmetic mean of events in each bin.

        Returns
        -------
        :
            The mean of each of the input bins.

        See Also
        --------
        scipp.mean:
            For calculating the mean of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_mean, self._obj)

    def nanmean(self) -> Variable | DataArray:
        """Arithmetic mean of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The mean of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmean:
            For calculating the mean of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmean, self._obj)

    def max(self) -> Variable | DataArray:
        """Maximum of events in each bin.

        Returns
        -------
        :
            The maximum of each of the input bins.

        See Also
        --------
        scipp.max:
            For calculating the maximum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_max, self._obj)

    def nanmax(self) -> Variable | DataArray:
        """Maximum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The maximum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmax:
            For calculating the maximum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmax, self._obj)

    def min(self) -> Variable | DataArray:
        """Minimum of events in each bin.

        Returns
        -------
        :
            The minimum of each of the input bins.

        See Also
        --------
        scipp.min:
            For calculating the minimum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_min, self._obj)

    def nanmin(self) -> Variable | DataArray:
        """Minimum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The minimum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmin:
            For calculating the minimum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmin, self._obj)

    def all(self) -> Variable | DataArray:
        """Logical AND of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The AND of each of the input bins without NaN's.

        See Also
        --------
        scipp.all:
            For performing an AND of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_all, self._obj)

    def any(self) -> Variable | DataArray:
        """Logical OR of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The OR of each of the input bins without NaN's.

        See Also
        --------
        scipp.all:
            For performing an OR of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_any, self._obj)

    def size(self) -> Variable | DataArray:
        """Number of events or elements in a bin.

        Returns
        -------
        :
            The number of elements in each of the input bins.
        """
        return _call_cpp_func(_cpp.bin_sizes, self._obj)

    def concat(self, dim: Dims = None) -> Variable | DataArray:
        """Concatenate bins element-wise by concatenating bin contents along
        their internal bin dimension.

        This is a reduction operation similar to :py:func:`scipp.sum` but operates on
        binned data. Elements (bins) are concatenated along their internal dimension.

        Parameters
        ----------
        dim:
            Reduction dimension.

        Returns
        -------
        :
            All bins along `dim` concatenated into a single bin.
        """
        return concat_bins(self._obj, dim)

    def concatenate(
        self,
        other: Variable | DataArray,
        *,
        out: DataArray | None = None,
    ) -> Variable | DataArray:
        """Concatenate bins element-wise by concatenating bin contents along
        their internal bin dimension.

        The bins to concatenate are obtained element-wise from `self` and `other`.

        Parameters
        ----------
        other:
            Other input containing bins.
        out:
            Optional output buffer.

        Returns
        -------
        :
            The bins of the two inputs merged.

        Raises
        ------
        scipp.DTypeError
            If `other` is not binned data.
        """
        if out is None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
        else:
            if self._obj is out:
                _call_cpp_func(_cpp.buckets.append, self._obj, other)
            else:
                out = _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
            return out


def _bins(obj) -> Bins | None:
    """
    Returns helper :py:class:`scipp.Bins` allowing bin-wise operations
    to be performed or `None` if not binned data.
    """
    if _cpp.is_bins(obj):
        return Bins(obj)
    else:
        return None


def _set_bins(obj, bins: Bins) -> None:
    # Should only be used by __iadd__ and friends
    if obj is not bins._obj:
        raise ValueError("Cannot set bins with a new object")


def bins(
    *,
    data: VariableLike,
    dim: str,
    begin: Variable | None = None,
    end: Variable | None = None,
) -> Variable:
    """Create a binned variable from bin indices.

    The elements of the returned variable are "bins", defined as views into
    ``data``. The returned variable keeps and manages a copy of ``data``
    internally.

    The variables ``begin`` and ``end`` must have the same dims and shape and
    ``dtype=sc.DType.int64``. The output dims and shape are given by ``begin``.
    If only ``begin`` is given, each bucket is a slice containing a non-range
    slice of ``data`` at the given indices. If neither ``begin`` nor ``end``
    are given, the output has ``dims=[dim]`` and contains all non-range slices
    along that dimension.

    Parameters
    ----------
    begin:
        Optional begin indices of bins, used for slicing ``data``.
        If not provided each row of ``data`` is mapped to a different bin.
    end:
        Optional end indices of bins, used for slicing ``data``. If not
        provided, ``begin`` is used as starting offsets for each bin, i.e., the end of
        the Nth bin is set to the begin of the N+1st bin.
    dim:
        Dimension of ``data`` that will be sliced to obtain data for
        any given bin.
    data:
        A variable, data array, or dataset containing combined data of all bins.

    Returns
    -------
    :
        Variable containing data in bins.

    See Also
    --------
    scipp.bin:
        For creating DataArrays based on binning of coord value
        instead of explicitly given index ranges.
    """
    if any(isinstance(x, DataGroup) for x in [begin, end, data]):
        raise ValueError("`scipp.bins` does not support DataGroup arguments.")
    return _call_cpp_func(_cpp.bins, begin, end, dim, data)


def bins_like(x: VariableLike, fill_value: Variable) -> Variable:
    """Create a binned variable by "broadcasting" fill values to bins of given sizes.

    The dimensions and shape of ``fill_value`` must be such that they can be broadcast
    to those of ``x``. Each element of ``fill_value`` defines the values of all the bin
    elements of the corresponding bin. The output shares the bin indices of ``x``.

    Parameters
    ----------
    x:
        Binned variable or data array serving as prototype for bin sizes.
    fill_value:
        Fill values to use for the bins.

    Returns
    -------
    :
        Variable containing fill value in bins.
    """
    if isinstance(x, DataGroup) or isinstance(fill_value, DataGroup):
        raise ValueError("`scipp.bins_like` does not support DataGroup arguments.")
    var = x
    if not isinstance(x, Variable):
        var = var.data
    return _call_cpp_func(_cpp.bins_like, var, fill_value)
