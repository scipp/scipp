# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

from __future__ import annotations
from pathlib import Path
from typing import Union

from ..logging import get_logger
from ..typing import VariableLike


def _dtype_lut():
    from .._scipp.core import DType as d
    # For types understood by numpy we do not actually need this special
    # handling, but will do as we add support for other types such as
    # variable-length strings.
    dtypes = [
        d.float64, d.float32, d.int64, d.int32, d.bool, d.datetime64, d.string,
        d.Variable, d.DataArray, d.Dataset, d.VariableView, d.DataArrayView,
        d.DatasetView, d.vector3, d.linear_transform3, d.affine_transform3,
        d.translation3, d.rotation3
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


def _as_hdf5_type(a):
    import numpy as np
    if np.issubdtype(a.dtype, np.datetime64):
        return a.view(np.int64)
    return a


def collection_element_name(name, index):
    """
    Convert name into an ASCII string that can be used as an object name in HDF5.
    """
    ascii_name = name.replace('.', '&#46;').replace('/', '&#47;').encode(
        'ascii', 'xmlcharrefreplace').decode('ascii')
    return f'elem_{index:03d}_{ascii_name}'


class NumpyDataIO:

    @staticmethod
    def write(group, data):
        dset = group.create_dataset('values', data=_as_hdf5_type(data.values))
        if data.variances is not None:
            variances = group.create_dataset('variances', data=data.variances)
            dset.attrs['variances'] = variances.ref
        return dset

    @staticmethod
    def read(group, data):
        # h5py's read_direct method fails if any dim has zero size.
        # see https://github.com/h5py/h5py/issues/870
        if data.values.flags['C_CONTIGUOUS'] and data.values.size > 0:
            group['values'].read_direct(_as_hdf5_type(data.values))
        else:
            # Values of Eigen matrices are transposed
            data.values = group['values']
        if 'variances' in group and data.variances.size > 0:
            group['variances'].read_direct(data.variances)


class BinDataIO:

    @staticmethod
    def write(group, data):
        bins = data.bins.constituents
        buffer_len = bins['data'].sizes[bins['dim']]
        # Crude mechanism to avoid writing large buffers, e.g., from
        # overallocation or when writing a slice of a larger variable. The
        # copy causes some overhead, but so would the (much mor complicated)
        # solution to extract contents bin-by-bin. This approach will likely
        # need to be revisited in the future.
        if buffer_len > 1.5 * data.bins.size().sum().value:
            data = data.copy()
            bins = data.bins.constituents
        values = group.create_group('values')
        VariableIO.write(values.create_group('begin'), var=bins['begin'])
        VariableIO.write(values.create_group('end'), var=bins['end'])
        data_group = values.create_group('data')
        data_group.attrs['dim'] = bins['dim']
        HDF5IO.write(data_group, bins['data'])
        return values

    @staticmethod
    def read(group):
        from .._scipp import core as sc
        values = group['values']
        begin = VariableIO.read(values['begin'])
        end = VariableIO.read(values['end'])
        dim = values['data'].attrs['dim']
        data = HDF5IO.read(values['data'])
        return sc.bins(begin=begin, end=end, dim=dim, data=data)


class ScippDataIO:

    @staticmethod
    def write(group, data):
        values = group.create_group('values')
        if len(data.shape) == 0:
            HDF5IO.write(values, data.value)
        else:
            for i, item in enumerate(data.values):
                HDF5IO.write(values.create_group(f'value-{i}'), item)
        return values

    @staticmethod
    def read(group, data):
        values = group['values']
        if len(data.shape) == 0:
            data.value = HDF5IO.read(values)
        else:
            for i in range(len(data.values)):
                data.values[i] = HDF5IO.read(values[f'value-{i}'])


class StringDataIO:

    @staticmethod
    def write(group, data):
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
    def read(group, data):
        values = group['values']
        if len(data.shape) == 0:
            data.value = values[()]
        else:
            for i in range(len(data.values)):
                data.values[i] = values[i]


def _write_scipp_header(group, what):
    from .._scipp import __version__
    group.attrs['scipp-version'] = __version__
    group.attrs['scipp-type'] = what


def _check_scipp_header(group, what):
    if 'scipp-version' not in group.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file/group written by scipp.")
    if group.attrs['scipp-type'] != what:
        raise RuntimeError(
            f"Attempt to read {what}, found {group.attrs['scipp-type']}.")


def _data_handler_lut():
    from .._scipp.core import DType as d
    handler = {}
    for dtype in [
            d.float64, d.float32, d.int64, d.int32, d.bool, d.datetime64, d.vector3,
            d.linear_transform3, d.rotation3, d.translation3, d.affine_transform3
    ]:
        handler[str(dtype)] = NumpyDataIO
    for dtype in [d.VariableView, d.DataArrayView, d.DatasetView]:
        handler[str(dtype)] = BinDataIO
    for dtype in [d.Variable, d.DataArray, d.Dataset]:
        handler[str(dtype)] = ScippDataIO
    for dtype in [d.string]:
        handler[str(dtype)] = StringDataIO
    return handler


class VariableIO:
    _dtypes = _dtype_lut()
    _data_handlers = _data_handler_lut()

    @classmethod
    def _write_data(cls, group, data):
        return cls._data_handlers[str(data.dtype)].write(group, data)

    @classmethod
    def _read_data(cls, group, data):
        return cls._data_handlers[str(data.dtype)].read(group, data)

    @classmethod
    def write(cls, group, var):
        if var.dtype not in cls._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            get_logger().warning('Writing with dtype=%s not implemented, skipping.',
                                 var.dtype)
            return
        _write_scipp_header(group, 'Variable')
        dset = cls._write_data(group, var)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        if var.unit is not None:
            dset.attrs['unit'] = str(var.unit)
        return group

    @classmethod
    def read(cls, group):
        _check_scipp_header(group, 'Variable')
        from .._scipp import core as sc
        from .._scipp.core import DType as d
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = cls._dtypes[values.attrs['dtype']]
        if 'unit' in values.attrs:
            contents['unit'] = sc.Unit(values.attrs['unit'])
        else:
            contents['unit'] = None  # essential, otherwise default unit is used
        contents['with_variances'] = 'variances' in group
        if contents['dtype'] in [d.VariableView, d.DataArrayView, d.DatasetView]:
            var = BinDataIO.read(group)
        else:
            var = sc.empty(**contents)
            cls._read_data(group, var)
        return var


def _write_mapping(parent, mapping, override=None):
    if override is None:
        override = {}
    for i, name in enumerate(mapping):
        var_group_name = collection_element_name(name, i)
        if (g := override.get(name)) is not None:
            parent[var_group_name] = g
        else:
            g = VariableIO.write(group=parent.create_group(var_group_name),
                                 var=mapping[name])
            if g is None:
                del parent[var_group_name]
            else:
                g.attrs['name'] = str(name)


def _read_mapping(group, override=None):
    if override is None:
        override = {}
    return {
        g.attrs['name']:
        override[g.attrs['name']] if g.attrs['name'] in override else HDF5IO.read(g)
        for g in group.values()
    }


class DataArrayIO:

    @staticmethod
    def write(group, data, override=None):
        if override is None:
            override = {}
        _write_scipp_header(group, 'DataArray')
        group.attrs['name'] = data.name
        VariableIO.write(group.create_group('data'), var=data.data)
        views = [data.coords, data.masks, data.attrs]
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks', 'attrs'], views):
            subgroup = group.create_group(view_name)
            _write_mapping(subgroup, view, override.get(view_name))

    @staticmethod
    def read(group, override=None):
        _check_scipp_header(group, 'DataArray')
        if override is None:
            override = {}
        from ..core import DataArray
        contents = dict()
        contents['name'] = group.attrs['name']
        contents['data'] = VariableIO.read(group['data'])
        for category in ['coords', 'masks', 'attrs']:
            contents[category] = _read_mapping(group[category], override.get(category))
        return DataArray(**contents)


class DatasetIO:

    @staticmethod
    def write(group, data):
        _write_scipp_header(group, 'Dataset')
        coords = group.create_group('coords')
        _write_mapping(coords, data.coords)
        entries = group.create_group('entries')
        # We cannot use coords directly, since we need lookup by name. The key used as
        # group name includes an integer index which may differ when writing items and
        # is not sufficient.
        coords = {v.attrs['name']: v for v in coords.values()}
        for i, (name, da) in enumerate(data.items()):
            HDF5IO.write(entries.create_group(collection_element_name(name, i)),
                         da,
                         override={'coords': coords})

    @staticmethod
    def read(group):
        _check_scipp_header(group, 'Dataset')
        from ..core import Dataset
        coords = _read_mapping(group['coords'])
        return Dataset(coords=coords,
                       data=_read_mapping(group['entries'], override={'coords':
                                                                      coords}))


class HDF5IO:
    _handlers = dict(
        zip(['Variable', 'DataArray', 'Dataset'], [VariableIO, DataArrayIO, DatasetIO]))

    @classmethod
    def write(cls, group, data, **kwargs):
        name = data.__class__.__name__.replace('View', '')
        return cls._handlers[name].write(group, data, **kwargs)

    @classmethod
    def read(cls, group, **kwargs):
        return cls._handlers[group.attrs['scipp-type']].read(group, **kwargs)


def to_hdf5(obj: VariableLike, filename: Union[str, Path]):
    """
    Writes object out to file in hdf5 format.
    """
    import h5py
    with h5py.File(filename, 'w') as f:
        HDF5IO.write(f, obj)


def open_hdf5(filename: Union[str, Path]) -> VariableLike:
    import h5py
    with h5py.File(filename, 'r') as f:
        return HDF5IO.read(f)
