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


def referenced_name():
    from uuid import uuid4
    return f'/referenced/{uuid4()}'


def _not_referenced_groups(itr):
    return filter(lambda x: r'/referenced' != x.name, itr)


class NumpyDataIO:
    @staticmethod
    def write(group, data, reference=False, h5ref=None):
        from h5py.h5r import Reference
        var_h5ref = None
        if h5ref is None:
            name = referenced_name() if reference else 'values'
            h5ref = group.create_dataset(name=name, data=_as_hdf5_type(data.values)).ref
            if data.variances is not None:
                name = referenced_name() if reference else 'variances'
                var_h5ref = group.create_dataset(name, data=data.variances).ref
        elif 'variances' in group[h5ref].attrs:
            var_h5ref = group[h5ref].attrs['variances']

        if not reference:
            dset = group[h5ref]
            # group['values'] = dset  # this is a self reference in this case?
            if isinstance(var_h5ref, Reference):
                # var_dset = group[var_h5ref]
                dset.attrs['variances'] = var_h5ref
                # group['variances'] = var_dset  # also a self reference ...
        else:
            group.attrs['values'] = h5ref
            if isinstance(var_h5ref, Reference):
                group.attrs['variances'] = var_h5ref

        return h5ref

    @staticmethod
    def read(group, data, reference=False, contents=None):
        def read_obj(dset):
            # h5py's read_direct method fails if any dim has zero size.
            # see https://github.com/h5py/h5py/issues/870
            if data.values.flags['C_CONTIGUOUS'] and data.values.size > 0:
                dset.read_direct(_as_hdf5_type(data.values))
            else:
                data.values = dset
            if 'variances' in dset.attrs and data.variances.size > 0:
                group[dset.attrs['variances']].read_direct(data.variances)

        if reference:
            if contents is None:
                contents = dict()
            ref = group.attrs['values']
            if ref in contents:
                # if we've already read this array, don't read it again (its variances are part of contents too)
                data = contents[ref]
            else:
                read_obj(group[ref])
                contents[ref] = data
        else:
            read_obj(group['values'])
        return contents


class BinDataIO:
    @staticmethod
    def write(group, data, **kwargs):
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
        VariableIO.write(values.create_group('begin'), var=bins['begin'], **kwargs)
        VariableIO.write(values.create_group('end'), var=bins['end'], **kwargs)
        data_group = values.create_group('data')
        data_group.attrs['dim'] = bins['dim']
        HDF5IO.write(data_group, bins['data'], **kwargs)
        return values

    @staticmethod
    def read(group, contents=None, **kwargs):
        if contents is None:
            contents = {}
        from .._scipp import core as sc
        values = group['values']
        begin = VariableIO.read(values['begin'], **kwargs)
        end = VariableIO.read(values['end'], **kwargs)
        dim = values['data'].attrs['dim']
        data = HDF5IO.read(values['data'], **kwargs)
        return sc.bins(begin=begin, end=end, dim=dim, data=data), contents


class ScippDataIO:
    @staticmethod
    def write(group, data, **kwargs):
        values = group.create_group('values')
        if len(data.shape) == 0:
            HDF5IO.write(values, data.value, **kwargs)
        else:
            for i, item in enumerate(data.values):
                HDF5IO.write(values.create_group(f'value-{i}'), item, **kwargs)
        return values

    @staticmethod
    def read(group, data, contents=None, **kwargs):
        if contents is None:
            contents = dict()
        values = group['values']
        if len(data.shape) == 0:
            data.value = HDF5IO.read(values, **kwargs)
        else:
            for i in range(len(data.values)):
                data.values[i] = HDF5IO.read(values[f'value-{i}'], **kwargs)
        return contents


class StringDataIO:
    @staticmethod
    def write(group, data, **kwargs):
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
    def read(group, data, contents=None, **kwargs):
        if contents is None:
            contents = dict()
        values = group['values']
        if len(data.shape) == 0:
            data.value = values[()]
        else:
            for i in range(len(data.values)):
                data.values[i] = values[i]
        return contents


def _write_scipp_header(group, what, reference=False, **kwargs):
    from .._scipp import __version__
    group.attrs['scipp-version'] = __version__
    group.attrs['scipp-type'] = what
    _write_reference_attr(group, reference)


def _check_scipp_header(group, what):
    if 'scipp-version' not in group.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file/group written by scipp.")
    if group.attrs['scipp-type'] != what:
        raise RuntimeError(
            f"Attempt to read {what}, found {group.attrs['scipp-type']}.")
    return _read_reference_attr(group)


def _write_reference_attr(group, reference=False):
    if reference:
        group.attrs['scipp-uuid-reference'] = 1


def _read_reference_attr(group):
    return group.attrs.get('scipp-uuid-reference', 0) > 0


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
    def _write_data(cls, group, data, **kwargs):
        return cls._data_handlers[str(data.dtype)].write(group, data, **kwargs)

    @classmethod
    def _read_data(cls, group, data, **kwargs):
        return cls._data_handlers[str(data.dtype)].read(group, data, **kwargs)

    @classmethod
    def write(cls, group, var, **kwargs):
        from h5py.h5r import Reference
        if var.dtype not in cls._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            get_logger().warning('Writing with dtype=%s not implemented, skipping.',
                                 var.dtype)
            return
        _write_scipp_header(group, 'Variable', **kwargs)
        dset = cls._write_data(group, var, **kwargs)
        is_ref = isinstance(dset, Reference)
        if is_ref:
            # we need to set more object attributes, but also want to return the reference
            dset = group[dset]
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        if var.unit is not None:
            dset.attrs['unit'] = str(var.unit)
        if is_ref:
            # return the reference instead of the group -- the calling scope already has the group object anyway
            return dset.ref
        return group

    @classmethod
    def read(cls, group, contents=None, **kwargs):
        has_contents = contents is not None
        if not has_contents:
            contents = dict()
        reference = _check_scipp_header(group, 'Variable')
        from .._scipp import core as sc
        from .._scipp.core import DType as d

        values = group[group.attrs['values']] if reference else group['values']
        parts = {key: values.attrs[key] for key in ['dims', 'shape']}
        parts['dtype'] = cls._dtypes[values.attrs['dtype']]
        if 'unit' in values.attrs:
            parts['unit'] = sc.Unit(values.attrs['unit'])
        else:
            parts['unit'] = None  # essential, otherwise default unit is used
        parts['with_variances'] = 'variances' in group or 'variances' in values.attrs
        if parts['dtype'] in [d.VariableView, d.DataArrayView, d.DatasetView]:
            var, contents = BinDataIO.read(group, contents=contents, **kwargs)
        else:
            var = sc.empty(**parts)
            contents = cls._read_data(group, var, contents=contents, **kwargs)
        return (var, contents) if has_contents else var


class DataArrayIO:
    @staticmethod
    def write(group, data, reference=False, contents=None, **kwargs):
        from h5py.h5r import Reference
        if contents is None:
            contents = {}
        _write_scipp_header(group, 'DataArray')
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError("Cannot write object with invalid data.")
        VariableIO.write(group.create_group('data'), var=data.data, **kwargs)
        views = [data.coords, data.masks, data.attrs]
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks', 'attrs'], views):
            subgroup = group.create_group(view_name)
            if len(view):
                _write_reference_attr(subgroup, reference)
            for i, name in enumerate(view):
                var_group_name = collection_element_name(name, i)
                content_key = f"{view_name}_{name}"
                content = contents.get(content_key, None)

                g = VariableIO.write(group=subgroup.create_group(var_group_name),
                                     var=view[name], reference=reference, h5ref=content)
                if g is None:
                    del subgroup[var_group_name]
                else:
                    if isinstance(g, Reference):
                        contents[content_key] = g if reference else None
                    # refer to the group by name in case g is a Reference
                    subgroup[var_group_name].attrs['name'] = str(name)

        return contents

    @staticmethod
    def read(group, contents=None, **kwargs):
        has_contents = contents is not None
        if not has_contents:
            contents = {}
        reference = _check_scipp_header(group, 'DataArray')
        from ..core import DataArray
        parts = dict()
        parts['name'] = group.attrs['name']
        parts['data'], contents = VariableIO.read(group['data'], contents=contents, reference=reference, **kwargs)

        for category in ['coords', 'masks', 'attrs']:
            part = dict()
            reference = _read_reference_attr(group[category])
            for g in group[category].values():
                v, contents = VariableIO.read(g, contents=contents, reference=reference, **kwargs)
                part[g.attrs['name']] = v
            parts[category] = part
        da = DataArray(**parts)
        return (da, contents) if has_contents else da


class DatasetIO:
    @staticmethod
    def write(group, data, reference=True, **kwargs):
        _write_scipp_header(group, 'Dataset', reference=False)  # this group has no referenced data (lower levels might)
        # Slight redundancy here from writing aligned coords for each item,
        # but irrelevant for common case of 1D coords with 2D (or higher)
        # data. The advantage is that we can read individual dataset entries
        # directly as data arrays.
        contents = {}
        for i, (name, da) in enumerate(data.items()):
            contents = HDF5IO.write(group.create_group(collection_element_name(name, i)), da, reference=reference, contents=contents)

    @staticmethod
    def read(group, contents=None, **kwargs):
        _check_scipp_header(group, 'Dataset')
        from ..core import Dataset
        if contents is None:
            contents = dict()
        data = dict()
        for g in _not_referenced_groups(group.values()):
            if 'scipp-type' in g.attrs:
                # protect against attempting to read non-scipp type data
                data[g.attrs['name']], contents = HDF5IO.read(g, contents=contents, **kwargs)
        return Dataset(data=data)


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
