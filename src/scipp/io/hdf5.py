# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

from __future__ import annotations

from collections.abc import Callable, Mapping, MutableMapping
from io import BytesIO, StringIO
from os import PathLike
from typing import TYPE_CHECKING, Any, ClassVar, Protocol, TypeVar, cast

import numpy as np
import numpy.typing as npt

from ..core import (
    DataArray,
    DataGroup,
    Dataset,
    DType,
    DTypeError,
    Unit,
    Variable,
    bins,
    empty,
)
from ..logging import get_logger
from ..typing import VariableLike

if TYPE_CHECKING:
    import h5py as h5
else:
    h5 = Any


_ObjectType = TypeVar('_ObjectType')


class _ObjectIO(Protocol[_ObjectType]):
    """Reader and writer for a top-level object represented as an HDF5 group."""

    @staticmethod
    def write(group: h5.Group, data: _ObjectType) -> h5.Group | None: ...

    @staticmethod
    def read(group: h5.Group) -> _ObjectType | None: ...


class _VariableDataIO(Protocol):
    """Reader and writer for a Scipp variable's data."""

    @staticmethod
    def write(group: h5.Group, data: Variable) -> h5.Dataset | h5.Group: ...

    @staticmethod
    def read(group: h5.Group, data: Variable) -> None: ...


def _dtype_lut() -> dict[str, DType]:
    # For types understood by numpy we do not actually need this special
    # handling, but will do as we add support for other types such as
    # variable-length strings.
    dtypes = [
        DType.float64,
        DType.float32,
        DType.int64,
        DType.int32,
        DType.bool,
        DType.datetime64,
        DType.string,
        DType.Variable,
        DType.DataArray,
        DType.Dataset,
        DType.vector3,
        DType.linear_transform3,
        DType.affine_transform3,
        DType.translation3,
        DType.rotation3,
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes, strict=True))


def _as_hdf5_type(a: npt.NDArray[Any]) -> npt.NDArray[Any]:
    if np.issubdtype(a.dtype, np.datetime64):
        return a.view(np.int64)
    return a


def _collection_element_name(name: str, index: int) -> str:
    """
    Convert name into an ASCII string that can be used as an object name in HDF5.
    """
    ascii_name = (
        name.replace('.', '&#46;')
        .replace('/', '&#47;')
        .encode('ascii', 'xmlcharrefreplace')
        .decode('ascii')
    )
    return f'elem_{index:03d}_{ascii_name}'


class _NumpyDataIO:
    @staticmethod
    def write(group: h5.Group, data: Variable) -> h5.Dataset:
        dset = group.create_dataset('values', data=_as_hdf5_type(data.values))
        if data.variances is not None:
            variances = group.create_dataset('variances', data=data.variances)
            dset.attrs['variances'] = variances.ref
        return dset

    @staticmethod
    def read(group: h5.Group, data: Variable) -> None:
        # h5py's read_direct method fails if any dim has zero size.
        # see https://github.com/h5py/h5py/issues/870
        if data.values.flags['C_CONTIGUOUS'] and data.values.size > 0:
            group['values'].read_direct(_as_hdf5_type(data.values))
        else:
            # Values of Eigen matrices are transposed
            data.values = group['values']
        if 'variances' in group and data.variances.size > 0:
            group['variances'].read_direct(data.variances)


class _BinDataIO:
    @staticmethod
    def write(group: h5.Group, data: Variable) -> h5.Group:
        if not data.is_binned:
            raise DTypeError("Expected binned data")
        bins = data.bins.constituents
        buffer_len = bins['data'].sizes[bins['dim']]
        # Crude mechanism to avoid writing large buffers, e.g., from
        # overallocation or when writing a slice of a larger variable. The
        # copy causes some overhead, but so would the (much more complicated)
        # solution to extract contents bin-by-bin. This approach will likely
        # need to be revisited in the future.
        if buffer_len > 1.5 * data.bins.size().sum().value:
            data = data.copy()
            bins = data.bins.constituents
        values = group.create_group('values')
        _VariableIO.write(values.create_group('begin'), var=bins['begin'])
        _VariableIO.write(values.create_group('end'), var=bins['end'])
        data_group = values.create_group('data')
        data_group.attrs['dim'] = bins['dim']
        _HDF5IO.write(data_group, bins['data'])
        return values

    @staticmethod
    def read(group: h5.Group) -> Variable:
        values = group['values']
        begin = _VariableIO.read(values['begin'])
        end = _VariableIO.read(values['end'])
        dim = values['data'].attrs['dim']
        data = cast(Variable | DataArray, _HDF5IO.read(values['data']))
        return bins(begin=begin, end=end, dim=dim, data=data)


class _ScippDataIO:
    @staticmethod
    def write(group: h5.Group, data: Variable) -> h5.Group:
        values = group.create_group('values')
        if len(data.shape) == 0:
            _HDF5IO.write(values, data.value)
        else:
            for i, item in enumerate(data.values):
                _HDF5IO.write(values.create_group(f'value-{i}'), item)
        return values

    @staticmethod
    def read(group: h5.Group, data: Variable) -> None:
        values = group['values']
        if len(data.shape) == 0:
            data.value = _HDF5IO.read(values)
        else:
            for i in range(len(data.values)):
                data.values[i] = _HDF5IO.read(values[f'value-{i}'])


class _StringDataIO:
    @staticmethod
    def write(group: h5.Group, data: Variable) -> h5.Dataset:
        import h5py

        dt = h5py.string_dtype(encoding='utf-8')
        dset = group.create_dataset('values', shape=data.shape, dtype=dt)
        if len(data.shape) == 0:
            dset[()] = data.value
        else:
            for i in range(len(data.values)):
                dset[i] = data.values[i]
        return dset

    @staticmethod
    def read(group: h5.Group, data: Variable) -> None:
        values = group['values']
        if len(data.shape) == 0:
            data.value = values[()]
        else:
            for i in range(len(data.values)):
                data.values[i] = values[i]


def _write_scipp_header(group: h5.Group, what: str) -> None:
    from ..core import __version__

    group.attrs['scipp-version'] = __version__
    group.attrs['scipp-type'] = what


def _check_scipp_header(group: h5.Group, what: str) -> None:
    if 'scipp-version' not in group.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file/group written by Scipp."
        )
    if group.attrs['scipp-type'] != what:
        raise RuntimeError(
            f"Attempt to read {what}, found {group.attrs['scipp-type']}."
        )


def _array_data_io_lut() -> dict[str, _VariableDataIO]:
    handler: dict[str, _VariableDataIO] = {}
    for dtype in [
        DType.float64,
        DType.float32,
        DType.int64,
        DType.int32,
        DType.bool,
        DType.datetime64,
        DType.vector3,
        DType.linear_transform3,
        DType.rotation3,
        DType.translation3,
        DType.affine_transform3,
    ]:
        handler[str(dtype)] = _NumpyDataIO
    for dtype in [DType.Variable, DType.DataArray, DType.Dataset]:
        handler[str(dtype)] = _ScippDataIO
    for dtype in [DType.string]:
        handler[str(dtype)] = _StringDataIO
    return handler


def _serialize_unit(unit: Unit) -> npt.NDArray[Any]:
    unit_dict: Any = unit.to_dict()
    dtype: list[tuple[str, Any]] = [('__version__', int), ('multiplier', float)]
    vals = [unit_dict['__version__'], unit_dict['multiplier']]
    if 'powers' in unit_dict:
        dtype.append(('powers', [(name, int) for name in unit_dict['powers']]))
        vals.append(tuple(val for val in unit_dict['powers'].values()))
    return np.array(tuple(vals), dtype=dtype)


def _read_unit_attr(ds: h5.Dataset) -> Unit:
    u = ds.attrs['unit']
    if isinstance(u, str):
        return Unit(u)  # legacy encoding as a string

    # u is a structured numpy array
    unit_dict = {'__version__': u['__version__'], 'multiplier': u['multiplier']}
    if 'powers' in u.dtype.names:
        unit_dict['powers'] = {
            name: u['powers'][name] for name in u['powers'].dtype.names
        }
    return Unit.from_dict(unit_dict)


_binned_dtype_lut = {
    'Variable': str(DType.VariableView),
    'DataArray': str(DType.DataArrayView),
    'Dataset': str(DType.DatasetView),
}


class _VariableIO:
    _dtypes = _dtype_lut()
    _array_data_readers = _array_data_io_lut()
    _data_writers = _array_data_io_lut()

    @classmethod
    def _write_data(cls, group: h5.Group, data: Variable) -> h5.Dataset | h5.Group:
        return cls._data_writers[str(data.dtype)].write(group, data)

    @classmethod
    def _read_array_data(cls, group: h5.Group, data: Variable) -> None:
        cls._array_data_readers[str(data.dtype)].read(group, data)

    @classmethod
    def write(cls, group: h5.Group, var: Variable) -> h5.Group | None:
        if not var.is_binned and var.dtype not in cls._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            get_logger().warning(
                'Writing with dtype=%s not implemented, skipping.', var.dtype
            )
            return None
        _write_scipp_header(group, 'Variable')
        if var.is_binned:
            dset = _BinDataIO.write(group, var)
            dset.attrs['dtype'] = _binned_dtype_lut[dset['data'].attrs['scipp-type']]
        else:
            dset = cls._write_data(group, var)
            dset.attrs['dtype'] = str(var.dtype)
            if var.unit is not None:
                dset.attrs['unit'] = _serialize_unit(var.unit)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['aligned'] = var.aligned
        return group

    @classmethod
    def read(cls, group: h5.Group) -> Variable:
        _check_scipp_header(group, 'Variable')
        values = group['values']
        dtype_str = values.attrs['dtype']
        if dtype_str in _binned_dtype_lut.values():
            return _BinDataIO.read(group)
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = cls._dtypes[dtype_str]
        if 'unit' in values.attrs:
            contents['unit'] = _read_unit_attr(values)
        else:
            contents['unit'] = None  # essential, otherwise default unit is used
        contents['with_variances'] = 'variances' in group
        contents['aligned'] = values.attrs.get('aligned', True)
        var = empty(**contents)
        cls._read_array_data(group, var)
        return var


def _write_mapping(
    parent: h5.Group,
    mapping: Mapping[str, object],
    override: Mapping[str, h5.Group] | None = None,
) -> None:
    if override is None:
        override = {}
    for i, name in enumerate(mapping):
        var_group_name = _collection_element_name(name, i)
        if (g := override.get(name)) is not None:
            parent[var_group_name] = g
        else:
            g = _HDF5IO.write(
                group=parent.create_group(var_group_name), data=mapping[name]
            )
            if g is None:
                del parent[var_group_name]
            else:
                g.attrs['name'] = str(name)


def _read_mapping(
    group: h5.Group,
    override: Mapping[str, object] | None = None,
) -> Mapping[str, object]:
    if override is None:
        override = {}
    return {
        g.attrs['name']: override[g.attrs['name']]
        if g.attrs['name'] in override
        else _HDF5IO.read(g)
        for g in group.values()
    }


class _DataArrayIO:
    @staticmethod
    def write(
        group: h5.Group,
        data: DataArray,
        override: Mapping[str, Mapping[str, h5.Group]] | None = None,
    ) -> h5.Group | None:
        if override is None:
            override = {}
        _write_scipp_header(group, 'DataArray')
        group.attrs['name'] = data.name
        if _VariableIO.write(group.create_group('data'), var=data.data) is None:
            return None
        views = [data.coords, data.masks]
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks'], views, strict=True):
            subgroup = group.create_group(view_name)
            _write_mapping(subgroup, view, override.get(view_name))
        return group

    @staticmethod
    def read(
        group: h5.Group,
        override: Mapping[str, Mapping[str, VariableLike]] | None = None,
    ) -> DataArray:
        _check_scipp_header(group, 'DataArray')
        if override is None:
            override = {}
        contents = {}
        contents['name'] = group.attrs['name']
        contents['data'] = _VariableIO.read(group['data'])
        for category in ['coords', 'masks']:
            contents[category] = _read_mapping(group[category], override.get(category))
        da = DataArray(**contents)
        da = _DataArrayIO._read_legacy_attrs_into(da, group, override)
        return da

    @staticmethod
    def _read_legacy_attrs_into(
        da: DataArray, group: h5.Group, override: Mapping[str, Mapping[str, h5.Group]]
    ) -> DataArray:
        """Load attributes as coordinates.

        Attributes were removed in https://github.com/scipp/scipp/pull/3626
        but old files that contain attributes remain.
        """
        if (attrs_group := group.get('attrs')) is not None:
            attrs = _read_mapping(attrs_group, override.get('attrs'))
            if intersection := da.coords.keys() & attrs.keys():
                raise ValueError(
                    f"Data array '{da.name}' contains legacy attributes "
                    f'{intersection} which also exist as coordinates.'
                )
            da = da.assign_coords(cast(dict[str, Variable], attrs))
            for name in attrs:
                da.coords.set_aligned(name, False)
        return da


class _DatasetIO:
    @staticmethod
    def write(group: h5.Group, data: Dataset) -> h5.Group:
        _write_scipp_header(group, 'Dataset')
        coords = group.create_group('coords')
        _write_mapping(coords, data.coords)
        entries = group.create_group('entries')
        # We cannot use coords directly, since we need lookup by name. The key used as
        # group name includes an integer index which may differ when writing items and
        # is not sufficient.
        coords = {v.attrs['name']: v for v in coords.values()}
        for i, (name, da) in enumerate(data.items()):
            _DataArrayIO.write(
                entries.create_group(_collection_element_name(name, i)),
                da,
                override={'coords': coords},
            )
        return group

    @staticmethod
    def read(group: h5.Group) -> Dataset:
        _check_scipp_header(group, 'Dataset')
        coords = cast(dict[str, Variable], _read_mapping(group['coords']))
        override = {'coords': coords}
        data: MutableMapping[str, Variable | DataArray] = {}
        for g in group['entries'].values():
            name = g.attrs['name']
            if g.attrs['scipp-type'] == 'DataArray':
                data[name] = _DataArrayIO.read(g, override=override)
            else:
                data[name] = _VariableIO.read(g)
        return Dataset(coords=coords, data=data)


class _DataGroupIO:
    @staticmethod
    def write(group: h5.Group, data: DataGroup[object]) -> h5.Group:
        _write_scipp_header(group, 'DataGroup')
        entries = group.create_group('entries')
        _write_mapping(entries, data)
        return group

    @staticmethod
    def read(group: h5.Group) -> DataGroup[Any]:
        _check_scipp_header(group, 'DataGroup')
        return DataGroup(_read_mapping(group['entries']))


def _direct_io(
    cls: type[_ObjectType], convert: Callable[[Any], _ObjectType] | None = None
) -> _ObjectIO[_ObjectType]:
    # Use fully qualified name for numpy.bool to avoid confusion with builtin bool."
    # Numpy bools have different names in 1.x and 2.x
    # TODO: This should be fixed once we drop support for numpy<2
    if cls.__module__ == 'numpy' and cls.__name__[:4] == 'bool':
        type_name = 'numpy.bool'
    else:
        type_name = cls.__name__
    convert_fn: Callable[[Any], _ObjectType] = cls if convert is None else convert

    class GenericIO:
        @staticmethod
        def write(group: h5.Group, data: _ObjectType) -> h5.Group:
            _write_scipp_header(group, type_name)
            group['entry'] = data
            return group

        @staticmethod
        def read(group: h5.Group) -> _ObjectType:
            _check_scipp_header(group, type_name)
            return convert_fn(group['entry'][()])

    return GenericIO


class _HDF5IO:
    _handlers: ClassVar[dict[str, _ObjectIO[Any]]] = {
        'Variable': _VariableIO,
        'DataArray': _DataArrayIO,
        'Dataset': _DatasetIO,
        'DataGroup': _DataGroupIO,
        'str': _direct_io(str, convert=lambda b: b.decode('utf-8')),
        'ndarray': _direct_io(np.ndarray, convert=lambda x: x),
        'numpy.bool': _direct_io(np.bool_),
        **{
            cls.__name__: _direct_io(cls)
            for cls in (
                int,
                np.int64,
                np.int32,
                np.uint64,
                np.uint32,
                float,
                np.float32,
                np.float64,
                bool,
                bytes,
            )
        },
    }

    @classmethod
    def write(cls, group: h5.Group, data: object) -> h5.Group | None:
        data_cls = data.__class__
        # Numpy bools have different names in 1.x and 2.x
        # TODO: This should be fixed once we drop support for numpy<2
        if data_cls.__module__ == 'numpy' and data_cls.__name__[:4] == 'bool':
            name = 'numpy.bool'
        else:
            name = data_cls.__name__.replace('View', '')
        try:
            handler = cls._handlers[name]
        except KeyError:
            get_logger().warning(
                "Writing type '%s' to HDF5 not implemented, skipping.", type(data)
            )
            return None
        return handler.write(group, data)

    @classmethod
    def read(cls, group: h5.Group) -> object:
        return cls._handlers[group.attrs['scipp-type']].read(group)


def save_hdf5(
    obj: object,
    filename: str | PathLike[str] | StringIO | BytesIO | h5.Group,
) -> None:
    """Write an object out to file in HDF5 format.

    Supported types include :class:`Variable`, :class:`DataArray`,
    :class:`Dataset`, and :class:`DataGroup`. Nested structures are supported.

    Parameters
    ----------
    obj:
        The object to save. Can be a Variable, DataArray, Dataset, or DataGroup.
    filename:
        Path to the output file or an open HDF5 group.

    See Also
    --------
    scipp.io.load_hdf5:
        Load data from HDF5 files.

    Examples
    --------
    Save and load a Variable:

      >>> import scipp as sc
      >>> import tempfile
      >>> var = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> with tempfile.NamedTemporaryFile(suffix='.h5') as f:
      ...     sc.io.save_hdf5(var, f.name)
      ...     loaded = sc.io.load_hdf5(f.name)
      >>> loaded
      <scipp.Variable> (x: 3)    float64              [m]  [1, 2, 3]

    Save and load a DataArray with coordinates:

      >>> da = sc.DataArray(
      ...     sc.array(dims=['x'], values=[10, 20, 30], unit='counts'),
      ...     coords={'x': sc.array(dims=['x'], values=[0.1, 0.2, 0.3], unit='m')}
      ... )
      >>> with tempfile.NamedTemporaryFile(suffix='.h5') as f:
      ...     sc.io.save_hdf5(da, f.name)
      ...     loaded = sc.io.load_hdf5(f.name)
      >>> loaded
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                         float64              [m]  (x)  [0.1, 0.2, 0.3]
      Data:
                                    int64         [counts]  (x)  [10, 20, 30]
    """
    import h5py

    if isinstance(filename, h5py.Group):
        _HDF5IO.write(filename, obj)

    with h5py.File(filename, 'w') as f:
        _HDF5IO.write(f, obj)


def load_hdf5(
    filename: str | PathLike[str] | StringIO | BytesIO | h5.Group,
) -> object:
    """Load a Scipp-HDF5 file.

    The file must have been written by :func:`scipp.io.save_hdf5` or be
    compatible with Scipp's HDF5 format.

    Parameters
    ----------
    filename:
        Path to the input file or an open HDF5 group.

    Returns
    -------
    :
        The loaded object (Variable, DataArray, Dataset, or DataGroup).

    See Also
    --------
    scipp.io.save_hdf5:
        Save data to HDF5 files.

    Examples
    --------
    See :func:`scipp.io.save_hdf5` for examples of saving and loading data.
    """
    import h5py

    if isinstance(filename, h5py.Group):
        return _HDF5IO.read(filename)

    with h5py.File(filename, 'r') as f:
        return _HDF5IO.read(f)
