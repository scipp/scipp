# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from collections.abc import Callable
from typing import Generic, Literal, Sequence, TypedDict, TypeVar

from .._scipp import core as _cpp
from ..typing import Dims, MetaDataMap, VariableLike
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .argument_handlers import IntoStrDict, combine_dict_args
from .bin_remapping import concat_bins
from .cpp_classes import Coords, DataArray, Dataset, DType, Unit, Variable
from .data_group import DataGroup
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
        op: Callable[[DataArray, Variable, str, Variable | None], Variable],
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
    Create a lookup table from a histogram (bin-edge coordinates):

      >>> import scipp as sc
      >>> x = sc.linspace(dim='x', start=0.0, stop=1.0, num=4)
      >>> vals = sc.array(dims=['x'], values=[3, 2, 1])
      >>> hist = sc.DataArray(data=vals, coords={'x': x})
      >>> lut = sc.lookup(hist, 'x')
      >>> lut[sc.array(dims=['event'], values=[0.1, 0.4, 0.6, 0.9])]
      <scipp.Variable> (event: 4)      int64  [dimensionless]  [3, 2, 2, 1]

    For point data (non-histogram), use ``mode='previous'`` for step-like lookup:

      >>> x_points = sc.array(dims=['x'], values=[0.0, 0.25, 0.5, 0.75, 1.0], unit='s')
      >>> vals_points = sc.array(dims=['x'], values=[10, 20, 30, 40, 50])
      >>> timeseries = sc.DataArray(data=vals_points, coords={'x': x_points})
      >>> lut_prev = sc.lookup(timeseries, 'x', mode='previous')
      >>> query = sc.array(dims=['time'], values=[0.1, 0.3, 0.6, 0.9], unit='s')
      >>> lut_prev[query]
      <scipp.Variable> (time: 4)      int64  [dimensionless]  [10, 20, 30, 40]

    Or use ``mode='nearest'`` for nearest-neighbor interpolation:

      >>> lut_nearest = sc.lookup(timeseries, 'x', mode='nearest')
      >>> lut_nearest[query]
      <scipp.Variable> (time: 4)      int64  [dimensionless]  [10, 20, 30, 50]

    Use ``fill_value`` to specify the value for out-of-range points:

      >>> x = sc.linspace(dim='x', start=0.0, stop=1.0, num=4)
      >>> vals = sc.array(dims=['x'], values=[3.0, 2.0, 1.0])
      >>> hist = sc.DataArray(data=vals, coords={'x': x})
      >>> lut = sc.lookup(hist, 'x', fill_value=sc.scalar(-1.0))
      >>> lut[sc.array(dims=['event'], values=[-0.5, 0.5, 1.5])]
      <scipp.Variable> (event: 3)    float64  [dimensionless]  [-1, 2, -1]
    """
    if dim is None:
        dim = func.dim
    func = DataArray(func.data, coords={dim: func.coords[dim]}, masks=func.masks)
    if func.dims[-1] != dim:
        # We automatically transpose the data so that `dim` is the inner dimension to
        # ensure contiguous memory access.
        dims = (*[d for d in func.dims if d != dim], dim)
        func.data = func.data.transpose(dims).copy()
        func.coords[dim] = (
            func.coords[dim]
            .transpose([d for d in dims if d in func.coords[dim].dims])
            .copy()
        )
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
        if func.coords[dim].dtype in [DType.bool, DType.int32, DType.int64]:
            raise ValueError("For mode='nearest' the lookup coordinate must be float.")
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


_O = TypeVar("_O", Variable, DataArray, Dataset)
_T = TypeVar("_T", bound=Variable | DataArray | Dataset)


class Bins(Generic[_O]):
    """Proxy for access to bin contents and operations on bins of a variable.

    This class is returned from the `bins` property of variables and should
    generally not be created directly.

    ``Bins`` is generic over the parent type, *not* the event type.
    That is, ``Variable.bins`` always returns ``Bins[Variable]`` regardless of whether
    the event list is a variable or data array.
    """

    def __init__(self, obj: _O) -> None:
        self._obj: _O = obj

    def _data(self) -> Variable:
        if isinstance(self._obj, DataArray | Dataset):
            # Raises AttributeError for datasets as it should.
            return self._obj.data  # type: ignore[attr-defined, no-any-return]
        else:
            return self._obj

    def __mul__(self, lut: Lookup) -> _O:
        """
        Scale event data using a lookup table.

        Multiplies event values by the corresponding lookup table value based on
        the event's coordinate. This is commonly used for applying corrections,
        normalization factors, or efficiency curves to event data.

        Parameters
        ----------
        lut:
            Lookup table created with :py:func:`scipp.lookup`.

        Returns
        -------
        :
            Copy with scaled event values.

        See Also
        --------
        scipp.lookup:
            Create lookup tables from histograms or point data.

        Examples
        --------
        Apply bin-dependent correction factors to event data:

          >>> import scipp as sc
          >>> table = sc.data.table_xyz(100)
          >>> binned = table.bin(x=4)
          >>> binned.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.250414, ..., 0.744977, 0.992259]
          Data:
                                      float64              [K]  (x)  [26.2165, 29.6565, 18.8663, 30.3569]

        Create correction factors (one per bin):

          >>> correction = sc.DataArray(
          ...     data=sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0]),
          ...     coords={'x': binned.coords['x']}
          ... )
          >>> lut = sc.lookup(correction, 'x')

        Apply corrections - each event is scaled by the lookup value for its x coordinate:

          >>> scaled = binned.bins * lut
          >>> scaled.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.250414, ..., 0.744977, 0.992259]
          Data:
                                      float64              [K]  (x)  [26.2165, 59.3129, 56.5988, 121.428]
        """  # noqa: E501
        if isinstance(self._obj, Dataset):
            raise NotImplementedError(
                "Multiplication of events in a dataset is not implemented"
            )
        target_dtype = (
            scalar(1, dtype=self.dtype) * scalar(1, dtype=lut.func.dtype)
        ).dtype
        copy = self._obj.to(dtype=target_dtype)
        _cpp.buckets.scale(copy, lut.func, lut.dim)
        return copy

    def __truediv__(self, lut: Lookup) -> _O:
        if isinstance(self._obj, Dataset):
            raise NotImplementedError(
                "Division of events in a dataset is not implemented"
            )
        target_dtype = (
            scalar(1, dtype=self.dtype) / scalar(1, dtype=lut.func.dtype)
        ).dtype
        copy = self._obj.to(dtype=target_dtype)
        _cpp.buckets.scale(copy, _cpp.reciprocal(lut.func), lut.dim)
        return copy

    def __imul__(self, lut: Lookup) -> Bins[_O]:  # noqa: PYI034
        _cpp.buckets.scale(self._obj, lut.func, lut.dim)
        return self

    def __itruediv__(self, lut: Lookup) -> Bins[_O]:  # noqa: PYI034
        _cpp.buckets.scale(self._obj, _cpp.reciprocal(lut.func), lut.dim)
        return self

    def __getitem__(self, key: tuple[str, Variable | slice]) -> DataArray:
        """
        Extract events from bins based on labels or label ranges and return a copy.

        This is similar to regular label-based indexing, but considers the event-coords,
        i.e., the coord values of individual bin entries. Unlike normal label-based
        indexing this returns a copy, as a subset of events is extracted.

        Parameters
        ----------
        key:
            A tuple of (dimension label, selection). The selection can be:

            - A 0-D Variable: selects events matching that exact label value
            - A slice with Variable start/stop: selects events in that range

        Returns
        -------
        :
            DataArray with events matching the selection.

        Examples
        --------
        Slice events by coordinate range:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.size()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.250414, ...,
                                            0.744977, 0.992259]
          Data:
                                        int64        <no unit>  (x)  [25, 28, 18, 29]

        Extract events where x is between 0.2 and 0.5 m:

          >>> start = sc.scalar(0.2, unit='m')
          >>> stop = sc.scalar(0.5, unit='m')
          >>> sliced = binned.bins['x', start:stop]
          >>> sliced.bins.size()
          <scipp.DataArray>
          Dimensions: Sizes[]
          Coordinates:
            x  float64  [m]  (x [bin-edge])  [0.2, 0.5]
          Data:
                                        int64        <no unit>  ()  35
        """
        if isinstance(self._obj, Dataset):
            raise NotImplementedError(
                "Extracting events from Datasets is not implemented."
            )

        dim, index = key
        if isinstance(index, Variable):
            if index.ndim == 0:
                if not isinstance(self._obj, DataArray):
                    raise NotImplementedError(
                        "Getting events by label is only implemented for DataArrays."
                    )
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
                start = self.coords[dim].min()
            if stop is None:
                stop = _upper_bound(self.coords[dim].max())

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

    def assign(self, data: Variable) -> _O:
        """Assign data variable to bins, if content is a DataArray.

        Parameters
        ----------
        data:
            Data to assign to the bins content.

        Returns
        -------
        :
            The input with the new data assigned.

        Examples
        --------
        Replace the data in bins with new values:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> binned.bins.sum().data
          <scipp.Variable> (x: 2)    float64              [K]  [6, 4]

        Assign scaled data:

          >>> new_data = binned.bins.data * 10.0
          >>> result = binned.bins.assign(new_data)
          >>> result.bins.sum().data
          <scipp.Variable> (x: 2)    float64              [K]  [60, 40]
        """  # noqa: E501
        if isinstance(self._obj, Dataset):
            raise NotImplementedError("bins.assign does not support datasets")
        out = self._map_constituents_data(lambda data: data)
        out.bins.data = data
        return out

    @property
    def coords(self) -> Coords:
        """Coords of the bins.

        Examples
        --------
        Access event coordinates from binned data:

          >>> import scipp as sc
          >>> table = sc.data.table_xyz(20)
          >>> binned = table.bin(x=3)
          >>> binned.bins.coords  # doctest: +SKIP
          <scipp.Dict>
            x: <scipp.Variable> (row: 20)    float64              [m]  [...]
            y: <scipp.Variable> (row: 20)    float64              [m]  [...]
            z: <scipp.Variable> (row: 20)    float64              [m]  [...]

        Access a specific event coordinate:

          >>> 'x' in binned.bins.coords
          True
        """
        return _cpp._bins_view(self._data()).coords  # type: ignore[no-any-return]

    def assign_coords(
        self, coords: IntoStrDict[Variable] | None = None, /, **coords_kwargs: Variable
    ) -> _O:
        """Return a new object with coords assigned to bin content.

        Parameters
        ----------
        coords:
            Dict of coordinates to assign.
        **coords_kwargs:
            Coordinates as keyword arguments.

        Returns
        -------
        :
            New object with the coordinates assigned.

        Examples
        --------
        Add a derived coordinate to event data:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> new_coord = binned.bins.coords['x'] * 2.0
          >>> result = binned.bins.assign_coords(x_doubled=new_coord)
          >>> result.bins.coords
          <scipp.Dict>
            x: <scipp.Variable> (event: 4)    float64              [m]  [0.1, 0.3, 0.5, 0.9]
            x_doubled: <scipp.Variable> (event: 4)    float64              [m]  [0.2, 0.6, 1, 1.8]
        """  # noqa: E501
        # Shallow copy constituents
        out = self._map_constituents_data(lambda data: data)
        for name, coord in combine_dict_args(coords, coords_kwargs).items():
            out.bins.coords[name] = coord
        return out

    def drop_coords(self, coords: str | Sequence[str]) -> _O:
        """Return a new object with coords dropped from bin content.

        Parameters
        ----------
        coords:
            Name or names of coordinates to drop.

        Returns
        -------
        :
            New object with the coordinates dropped.

        Examples
        --------
        Remove an event coordinate:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> 'x' in binned.bins.coords
          True

          >>> result = binned.bins.drop_coords('x')
          >>> 'x' in result.bins.coords
          False
        """  # noqa: E501
        if isinstance(self._obj, Dataset):
            raise NotImplementedError("bins.drop_coords does not support datasets")
        return self._map_constituents_data(lambda data: data.drop_coords(coords))

    @property
    def masks(self) -> MetaDataMap:
        """Masks of the bins.

        Examples
        --------
        Access event masks from binned data:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> table.masks['quality'] = sc.array(dims=['event'], values=[False, True, False, False])
          >>> binned = table.bin(x=2)
          >>> binned.bins.masks
          <scipp.Dict>
            quality: <scipp.Variable> (event: 4)       bool        <no unit>  [False, True, False, False]

        Check if an event mask exists:

          >>> 'quality' in binned.bins.masks
          True
        """  # noqa: E501
        return _cpp._bins_view(self._data()).masks  # type: ignore[no-any-return]

    def assign_masks(
        self, masks: IntoStrDict[Variable] | None = None, /, **masks_kwargs: Variable
    ) -> _O:
        """Return a new object with masks assigned to bin content.

        Parameters
        ----------
        masks:
            Dict of masks to assign.
        **masks_kwargs:
            Masks as keyword arguments.

        Returns
        -------
        :
            New object with the masks assigned.

        Examples
        --------
        Add a mask to event data based on coordinate values:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> mask = binned.bins.coords['x'] > sc.scalar(0.5, unit='m')
          >>> result = binned.bins.assign_masks(high_x=mask)
          >>> result.bins.masks
          <scipp.Dict>
            high_x: <scipp.Variable> (event: 4)       bool        <no unit>  [False, False, False, True]
        """  # noqa: E501
        # Shallow copy constituents
        out = self._map_constituents_data(lambda data: data)
        for name, coord in combine_dict_args(masks, masks_kwargs).items():
            out.bins.masks[name] = coord
        return out

    def drop_masks(self, masks: str | Sequence[str]) -> _O:
        """Return a new object with masks dropped from bin content.

        Parameters
        ----------
        masks:
            Name or names of masks to drop.

        Returns
        -------
        :
            New object with the masks dropped.

        Examples
        --------
        Remove an event mask:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> table.masks['quality'] = sc.array(dims=['event'], values=[False, True, False, False])
          >>> binned = table.bin(x=2)
          >>> binned.bins.masks
          <scipp.Dict>
            quality: <scipp.Variable> (event: 4)       bool        <no unit>  [False, True, False, False]

          >>> result = binned.bins.drop_masks('quality')
          >>> result.bins.masks
          <scipp.Dict>
        """  # noqa: E501
        if isinstance(self._obj, Dataset):
            raise NotImplementedError("bins.drop_masks does not support datasets")
        return self._map_constituents_data(lambda data: data.drop_masks(masks))

    @property
    def data(self) -> Variable:
        """Data of the bins.

        Examples
        --------
        Access event data from binned data:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(50, 3)
          >>> binned.bins.data
          <scipp.Variable> (x: 3)  VariableView        <no unit>  binned data: dim='row', content=Variable(dims=(row: 50), dtype=float64, unit=K)

        Modify event data:

          >>> modified = binned.copy()
          >>> modified.bins.data = modified.bins.data * 2.0
          >>> modified.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          Data:
                                      float64              [K]  (x)  [37.8384, 25.3648, 41.7971]
        """  # noqa: E501
        return _cpp._bins_view(self._data()).data  # type: ignore[no-any-return]

    @data.setter
    def data(self, data: Variable) -> None:
        """Set data of the bins"""
        _cpp._bins_view(self._data()).data = data

    @property
    def unit(self) -> Unit | None:
        """Unit of the bin elements.

        Examples
        --------
        Get the unit of the data in the bins:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> binned.bins.unit
          Unit(K)
        """  # noqa: E501
        return self.constituents['data'].unit  # type: ignore[union-attr]

    @unit.setter
    def unit(self, unit: Unit | str | None) -> None:
        """Set unit of the bin elements"""
        self.constituents['data'].unit = unit  # type: ignore[union-attr]

    @property
    def dtype(self) -> DType:
        """Data type of the bin elements.

        Examples
        --------
        Get the data type of the bin contents:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> binned.bins.dtype
          DType('float64')

        Integer data:

          >>> int_var = sc.array(dims=['event'], values=[1, 2, 3, 4])
          >>> int_table = sc.DataArray(int_var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> int_binned = int_table.bin(x=2)
          >>> int_binned.bins.dtype
          DType('int64')
        """  # noqa: E501
        return self.constituents['data'].dtype  # type: ignore[union-attr]

    @property
    def aligned(self) -> bool:
        """Alignment flag for coordinates of bin elements."""
        return self.constituents['data'].aligned  # type: ignore[union-attr]

    @property
    def constituents(self) -> Constituents:
        """Constituents of binned data, as supported by :py:func:`sc.bins`.

        Returns a dict containing:
        - 'data': The underlying event data (Variable/DataArray)
        - 'begin': Variable with bin start indices
        - 'end': Variable with bin end indices
        - 'dim': Dimension name that the binning applies to

        Examples
        --------
        Access the underlying structure of binned data:

          >>> import scipp as sc
          >>> var = sc.array(dims=['event'], values=[1.0, 2.0, 3.0, 4.0], unit='K')
          >>> table = sc.DataArray(var, coords={'x': sc.array(dims=['event'], values=[0.1, 0.3, 0.5, 0.9], unit='m')})
          >>> binned = table.bin(x=2)
          >>> parts = binned.bins.constituents
          >>> parts['dim']
          'event'
          >>> parts['begin']
          <scipp.Variable> (x: 2)      int64        <no unit>  [0, 3]
          >>> parts['end']
          <scipp.Variable> (x: 2)      int64        <no unit>  [3, 4]
          >>> parts['data']
          <scipp.DataArray>
          Dimensions: Sizes[event:4, ]
          Coordinates:
          * x                         float64              [m]  (event)  [0.1, 0.3, 0.5, 0.9]
          Data:
                                      float64              [K]  (event)  [1, 2, 3, 4]

        Reconstruct binned data from constituents:

          >>> reconstructed = sc.bins(**parts)
          >>> sc.identical(binned.data, reconstructed)
          True
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_constituents, self._data())  # type: ignore[return-value]

    def sum(self) -> _O:
        """Sum of events in each bin.

        Returns
        -------
        :
            The sum of each of the input bins.

        See Also
        --------
        scipp.sum:
            For summing non-bin data or summing bins.

        Examples
        --------
        Sum events within each bin:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.250414, ..., 0.744977, 0.992259]
          Data:
                                    float64              [K]  (x)  [26.2165, 29.6565, 18.8663, 30.3569]

        Works with multidimensional binned data:

          >>> binned_2d = sc.data.binned_xy(100, 3, 2)
          >>> binned_2d.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, y:2, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          * y                         float64              [m]  (y [bin-edge])  [0.00675377, 0.499126, 0.991499]
          Data:
                                      float64              [K]  (x, y)  [15.8201, 23.0968, ..., 17.8679, 20.8697]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_sum, self._obj)  # type: ignore[return-value]

    def nansum(self) -> _O:
        """Sum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The sum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nansum:
            For summing non-bin data or summing bins.

        Examples
        --------
        With NaN values in the data, ``sum`` propagates NaN while ``nansum`` ignores them:

          >>> import scipp as sc
          >>> import numpy as np
          >>> from numpy.random import default_rng
          >>> rng = default_rng(seed=1234)
          >>> x = sc.array(dims=['row'], values=rng.random(50), unit='m')
          >>> data = sc.array(dims=['row'], values=rng.random(50), unit='K')
          >>> data.values[0] = np.nan  # inject NaN values
          >>> data.values[10] = np.nan
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)

        Regular sum produces NaN where bins contain NaN values:

          >>> binned.bins.sum()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          Data:
                                      float64              [K]  (x)  [10.7675, nan, nan]

        Using nansum ignores NaN values:

          >>> binned.bins.nansum()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          Data:
                                      float64              [K]  (x)  [10.7675, 5.26314, 7.32089]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_nansum, self._obj)  # type: ignore[return-value]

    def mean(self) -> _O:
        """Arithmetic mean of events in each bin.

        Returns
        -------
        :
            The mean of each of the input bins.

        See Also
        --------
        scipp.mean:
            For calculating the mean of non-bin data or across bins.

        Examples
        --------
        Compute the mean of events within each bin:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.mean()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.250414, ..., 0.744977, 0.992259]
          Data:
                                    float64              [K]  (x)  [1.04866, 1.05916, 1.04813, 1.04679]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_mean, self._obj)  # type: ignore[return-value]

    def nanmean(self) -> _O:
        """Arithmetic mean of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The mean of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmean:
            For calculating the mean of non-bin data or across bins.

        Examples
        --------
        With NaN values in the data, ``mean`` propagates NaN while ``nanmean`` ignores them:

          >>> import scipp as sc
          >>> import numpy as np
          >>> from numpy.random import default_rng
          >>> rng = default_rng(seed=1234)
          >>> x = sc.array(dims=['row'], values=rng.random(50), unit='m')
          >>> data = sc.array(dims=['row'], values=rng.random(50), unit='K')
          >>> data.values[0] = np.nan  # inject NaN values
          >>> data.values[10] = np.nan
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)

        Regular mean produces NaN where bins contain NaN values:

          >>> binned.bins.mean()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          Data:
                                      float64              [K]  (x)  [0.598195, nan, nan]

        Using nanmean ignores NaN values:

          >>> binned.bins.nanmean()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255, 0.992259]
          Data:
                                      float64              [K]  (x)  [0.598195, 0.478468, 0.38531]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_nanmean, self._obj)  # type: ignore[return-value]

    def max(self) -> _O:
        """Maximum of events in each bin.

        Returns
        -------
        :
            The maximum of each of the input bins.

        See Also
        --------
        scipp.max:
            For calculating the maximum of non-bin data or across bins.

        Examples
        --------
        Find the maximum event value within each bin:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.max()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.250414, ...,
                                            0.744977, 0.992259]
          Data:
                                      float64  [K]  (x)  [1.09436, 1.09533,
                                                                 1.09236, 1.09963]
        """
        return _call_cpp_func(_cpp.bins_max, self._obj)  # type: ignore[return-value]

    def nanmax(self) -> _O:
        """Maximum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The maximum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmax:
            For calculating the maximum of non-bin data or across bins.

        Examples
        --------
        With NaN values, ``max`` propagates NaN while ``nanmax`` ignores them:

          >>> import scipp as sc
          >>> import numpy as np
          >>> from numpy.random import default_rng
          >>> rng = default_rng(seed=1234)
          >>> x = sc.array(dims=['row'], values=rng.random(50), unit='m')
          >>> data = sc.array(dims=['row'], values=rng.random(50), unit='K')
          >>> data.values[0] = np.nan
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)
          >>> binned.bins.nanmax()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255,
                                            0.992259]
          Data:
                                      float64  [K]  (x)  [0.988635, 0.873824,
                                                                 0.893008]
        """
        return _call_cpp_func(_cpp.bins_nanmax, self._obj)  # type: ignore[return-value]

    def min(self) -> _O:
        """Minimum of events in each bin.

        Returns
        -------
        :
            The minimum of each of the input bins.

        See Also
        --------
        scipp.min:
            For calculating the minimum of non-bin data or across bins.

        Examples
        --------
        Find the minimum event value within each bin:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.min()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.250414, ...,
                                            0.744977, 0.992259]
          Data:
                                      float64  [K]  (x)  [1.00044, 1.00706,
                                                                 1.00387, 1.00136]
        """
        return _call_cpp_func(_cpp.bins_min, self._obj)  # type: ignore[return-value]

    def nanmin(self) -> _O:
        """Minimum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The minimum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmin:
            For calculating the minimum of non-bin data or across bins.

        Examples
        --------
        With NaN values, ``min`` propagates NaN while ``nanmin`` ignores them:

          >>> import scipp as sc
          >>> import numpy as np
          >>> from numpy.random import default_rng
          >>> rng = default_rng(seed=1234)
          >>> x = sc.array(dims=['row'], values=rng.random(50), unit='m')
          >>> data = sc.array(dims=['row'], values=rng.random(50), unit='K')
          >>> data.values[0] = np.nan
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)
          >>> binned.bins.nanmin()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.332841, 0.66255,
                                            0.992259]
          Data:
                                      float64  [K]  (x)  [0.0670407, 0.198358,
                                                                 0.00597817]
        """
        return _call_cpp_func(_cpp.bins_nanmin, self._obj)  # type: ignore[return-value]

    def all(self) -> _O:
        """Logical AND of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The AND of each of the input bins without NaN's.

        See Also
        --------
        scipp.all:
            For performing an AND of non-bin data or across bins.

        Examples
        --------
        Check if all events in each bin satisfy a condition:

          >>> import scipp as sc
          >>> x = sc.array(dims=['row'], values=[0.1, 0.15, 0.2, 0.4, 0.5, 0.6,
          ...                                     0.7, 0.8, 0.9], unit='m')
          >>> data = sc.array(dims=['row'], values=[True, True, True,
          ...                                        True, False, True,
          ...                                        False, False, False])
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)
          >>> binned.bins.all()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.1, 0.366667, 0.633333, 0.9]
          Data:
                                         bool        <no unit>  (x)  [True, False, False]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_all, self._obj)  # type: ignore[return-value]

    def any(self) -> _O:
        """Logical OR of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The OR of each of the input bins without NaN's.

        See Also
        --------
        scipp.any:
            For performing an OR of non-bin data or across bins.

        Examples
        --------
        Check if any event in each bin satisfies a condition:

          >>> import scipp as sc
          >>> x = sc.array(dims=['row'], values=[0.1, 0.15, 0.2, 0.4, 0.5, 0.6,
          ...                                     0.7, 0.8, 0.9], unit='m')
          >>> data = sc.array(dims=['row'], values=[True, True, True,
          ...                                        True, False, True,
          ...                                        False, False, False])
          >>> table = sc.DataArray(data, coords={'x': x})
          >>> binned = table.bin(x=3)
          >>> binned.bins.any()
          <scipp.DataArray>
          Dimensions: Sizes[x:3, ]
          Coordinates:
          * x                         float64              [m]  (x [bin-edge])  [0.1, 0.366667, 0.633333, 0.9]
          Data:
                                         bool        <no unit>  (x)  [True, True, False]
        """  # noqa: E501
        return _call_cpp_func(_cpp.bins_any, self._obj)  # type: ignore[return-value]

    def size(self) -> Variable:
        """Number of events or elements in a bin.

        Returns
        -------
        :
            The number of elements in each of the input bins.

        Examples
        --------
        Count the number of events in each bin:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> sizes = binned.bins.size()
          >>> sizes.dims
          ('x',)
          >>> sizes.data.dtype == sc.DType.int64
          True

        This is useful for checking bin populations, for example to filter empty bins:

          >>> mask = sc.array(dims=sizes.dims, values=sizes.values > 0)
          >>> non_empty = binned[mask]
        """
        return _call_cpp_func(_cpp.bin_sizes, self._obj)  # type: ignore[return-value]

    def concat(self, dim: Dims = None) -> _O:
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

        Examples
        --------
        Concatenate all bins along a dimension:

          >>> import scipp as sc
          >>> binned = sc.data.binned_x(100, 4)
          >>> binned.bins.size()
          <scipp.DataArray>
          Dimensions: Sizes[x:4, ]
          Coordinates:
          * x  float64  [m]  (x [bin-edge])  [0.00313229, 0.250414, ...,
                                            0.744977, 0.992259]
          Data:
                                        int64        <no unit>  (x)  [25, 28, 18, 29]

          >>> concatenated = binned.bins.concat('x')
          >>> concatenated.bins.size()
          <scipp.DataArray>
          Dimensions: Sizes[]
          Data:
                                        int64        <no unit>  ()  100

        This produces a scalar result with all events in a single bin.
        """
        if isinstance(self._obj, Dataset):
            raise NotImplementedError(
                "Concatenating bins is not implemented for datasets"
            )
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

        Examples
        --------
        Concatenate corresponding bins from two arrays element-wise:

          >>> import scipp as sc
          >>> table1 = sc.data.table_xyz(30)
          >>> table2 = sc.data.table_xyz(20)
          >>> x_edges = sc.linspace('x', 0.0, 1.0, 3, unit='m')
          >>> binned1 = table1.bin(x=x_edges)
          >>> binned2 = table2.bin(x=x_edges)
          >>> binned1.bins.size().values
          array([15, 15])
          >>> binned2.bins.size().values
          array([10, 10])

        Merge bins element-wise:

          >>> result = binned1.bins.concatenate(binned2)
          >>> result.bins.size().values
          array([25, 25])

        Each bin in the result contains events from the corresponding bins in both inputs.
        """  # noqa: E501
        if out is None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)  # type: ignore[return-value]
        else:
            if self._obj is out:
                _call_cpp_func(_cpp.buckets.append, self._obj, other)
            else:
                out = _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)  # type: ignore[assignment]
            return out

    def _map_constituents_data(self, f: Callable[[_T], _T]) -> _O:
        content = self.constituents
        content['data'] = f(content['data'])  # type: ignore[arg-type]
        data: Variable = _cpp._bins_no_validate(**content)
        if isinstance(self._obj, DataArray):
            return self._obj.assign(data)
        elif isinstance(self._obj, Dataset):
            raise NotImplementedError("Dataset events not supported")
        return data


def _is_binned(obj: _O) -> bool:
    """Return True if the object is binned."""
    return _cpp.is_bins(obj)  # type: ignore[no-any-return]


def _bins(obj: _O) -> Bins[_O]:
    """Returns helper :class:`scipp.Bins` for bin-wise operations.

    .. deprecated:: 25.11.0
       ``bins`` currently returns ``None`` if the object is not binned.
       In the future, this will change and ``bins`` will raise a
       :class:`scipp.BinnedDataError` instead.
       Use ``x.is_binned`` instead of ``x.bins is not None`` to check
       if ``x`` contains binned data.
    """
    if _cpp.is_bins(obj):
        return Bins(obj)
    else:
        return None  # type: ignore[return-value]


def _set_bins(obj: _O, bins: Bins[_O]) -> None:
    # Should only be used by __iadd__ and friends
    if obj is not bins._obj:
        raise ValueError("Cannot set bins with a new object")


def bins(
    *,
    data: VariableLike,
    dim: str,
    begin: Variable | None = None,
    end: Variable | None = None,
    validate_indices: bool = True,
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
    validate_indices:
        If True (default), validates that all indices are within bounds of the data and
        that bins are not overlapping.
        If False, skips validation for better performance but may cause undefined
        behavior if indices are out of bounds or overlapping.
        USE WITH EXTREME CAUTION and only if you know what you are doing!
        An example of safe usage of this option is reconstruction of binned data from
        the result of a `obj.bins.constituents` call.

    Returns
    -------
    :
        Variable containing data in bins.

    See Also
    --------
    scipp.bin:
        For creating DataArrays based on binning of coord value
        instead of explicitly given index ranges.

    Examples
    --------
    Create bins where each element becomes its own bin (no explicit indices):

      >>> import scipp as sc
      >>> data = sc.arange('row', 5, unit='m')
      >>> sc.bins(data=data, dim='row')
      <scipp.Variable> (row: 5)  VariableView        <no unit>  binned data: dim='row', content=...

    Create bins with explicit begin indices. Each bin contains elements from
    its begin index up to (but not including) the next bin's begin index:

      >>> data = sc.arange('row', 10, unit='m')
      >>> begin = sc.Variable(dims=['bin'], values=[0, 3, 7], dtype='int64', unit=None)
      >>> sc.bins(data=data, dim='row', begin=begin)
      <scipp.Variable> (bin: 3)  VariableView        <no unit>  binned data: dim='row', content=...

    Create bins with explicit begin and end indices for full control:

      >>> begin = sc.Variable(dims=['bin'], values=[0, 3, 7], dtype='int64', unit=None)
      >>> end = sc.Variable(dims=['bin'], values=[3, 7, 10], dtype='int64', unit=None)
      >>> binned = sc.bins(data=data, dim='row', begin=begin, end=end)
      >>> binned.sizes
      {'bin': 3}
    """  # noqa: E501
    if any(isinstance(x, DataGroup) for x in [begin, end, data]):
        raise ValueError("`scipp.bins` does not support DataGroup arguments.")

    bin_func = _cpp.bins if validate_indices else _cpp._bins_no_validate
    return _call_cpp_func(bin_func, begin, end, dim, data)  # type: ignore[return-value]


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

    Examples
    --------
    Create bins with constant fill values matching the structure of existing binned data:

      >>> import scipp as sc
      >>> binned = sc.data.binned_x(20, 3)
      >>> binned.bins.size()
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                         float64              [m]  (x [bin-edge])  [0.118091, 0.404294, 0.690497, 0.9767]
      Data:
                                    int64        <no unit>  (x)  [9, 4, 7]

    Fill each bin with a different value:

      >>> fill = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='K')
      >>> sc.bins_like(binned.data, fill)
      <scipp.Variable> (x: 3)  VariableView        <no unit>  binned data: dim='row', content=...

    This is useful for initializing binned data structures with known values.
    """  # noqa: E501
    if isinstance(x, DataGroup) or isinstance(fill_value, DataGroup):  # type: ignore[unreachable]
        raise ValueError("`scipp.bins_like` does not support DataGroup arguments.")
    if isinstance(x, Dataset) or isinstance(fill_value, Dataset):  # type: ignore[unreachable]
        raise ValueError("`scipp.bins_like` does not support Dataset arguments.")
    var = x.data if isinstance(x, DataArray) else x
    return _call_cpp_func(_cpp.bins_like, var, fill_value)  # type: ignore[return-value]
