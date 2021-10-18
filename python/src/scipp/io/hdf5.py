# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

from __future__ import annotations
from pathlib import Path
from typing import Union

from ..typing import VariableLike


def _dtype_lut():
    from .._scipp.core import dtype as d
    # For types understood by numpy we do not actually need this special
    # handling, but will do as we add support for other types such as
    # variable-length strings.
    dtypes = [
        d.float64, d.float32, d.int64, d.int32, d.bool, d.datetime64, d.string,
        d.Variable, d.DataArray, d.Dataset, d.VariableView, d.DataArrayView,
        d.DatasetView, d.vector_3_float64, d.matrix_3_float64
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


def _as_hdf5_type(a):
    import numpy as np
    if np.issubdtype(a.dtype, np.datetime64):
        return a.view(np.int64)
    return a


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
    from .._scipp.core import dtype as d
    handler = {}
    for dtype in [
            d.float64, d.float32, d.int64, d.int32, d.bool, d.datetime64,
            d.vector_3_float64, d.matrix_3_float64
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
            print(f'Writing with dtype={var.dtype} not implemented, skipping.')
            return
        _write_scipp_header(group, 'Variable')
        dset = cls._write_data(group, var)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        dset.attrs['unit'] = str(var.unit)
        return group

    @classmethod
    def read(cls, group):
        _check_scipp_header(group, 'Variable')
        from .._scipp import core as sc
        from .._scipp.core import dtype as d
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = cls._dtypes[values.attrs['dtype']]
        contents['unit'] = sc.Unit(values.attrs['unit'])
        contents['with_variances'] = 'variances' in group
        if contents['dtype'] in [d.VariableView, d.DataArrayView, d.DatasetView]:
            var = BinDataIO.read(group)
        else:
            var = sc.empty(**contents)
            cls._read_data(group, var)
        return var


class DataArrayIO:
    @staticmethod
    def write(group, data):
        _write_scipp_header(group, 'DataArray')
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError("Cannot write object with invalid data.")
        VariableIO.write(group.create_group('data'), var=data.data)
        views = [data.coords, data.masks, data.attrs]
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks', 'attrs'], views):
            subgroup = group.create_group(view_name)
            for name in view:
                g = VariableIO.write(group=subgroup.create_group(str(name)),
                                     var=view[name])
                if g is None:
                    del subgroup[str(name)]

    @staticmethod
    def read(group):
        _check_scipp_header(group, 'DataArray')
        from ..core import DataArray
        contents = dict()
        contents['name'] = group.attrs['name']
        contents['data'] = VariableIO.read(group['data'])
        for category in ['coords', 'masks', 'attrs']:
            contents[category] = dict()
            for name in group[category]:
                g = group[category][name]
                contents[category][name] = VariableIO.read(g)
        return DataArray(**contents)


class DatasetIO:
    @staticmethod
    def write(group, data):
        _write_scipp_header(group, 'Dataset')
        # Slight redundancy here from writing aligned coords for each item,
        # but irrelevant for common case of 1D coords with 2D (or higher)
        # data. The advantage is the we can read individual dataset entries
        # directly as data arrays.
        for name in data:
            HDF5IO.write(group.create_group(name), data[name])

    @staticmethod
    def read(group):
        _check_scipp_header(group, 'Dataset')
        from .._scipp import core as sc
        return sc.Dataset(data={name: HDF5IO.read(group[name]) for name in group})


class HDF5IO:
    _handlers = dict(
        zip(['Variable', 'DataArray', 'Dataset'], [VariableIO, DataArrayIO, DatasetIO]))

    @classmethod
    def write(cls, group, data):
        name = data.__class__.__name__.replace('View', '')
        return cls._handlers[name].write(group, data)

    @classmethod
    def read(cls, group):
        return cls._handlers[group.attrs['scipp-type']].read(group)


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
