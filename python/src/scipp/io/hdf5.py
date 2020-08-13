# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock


def _unit_lut():
    from .._scipp.core import units as u
    units = u.supported_units()
    names = [str(unit) for unit in units]
    return dict(zip(names, units))


def _dtype_lut():
    from .._scipp.core import dtype as d
    # For types understood by numpy we do not actually need this special
    # handling, but will do as we add support for other types such as
    # variable-length strings.
    dtypes = [
        d.float64, d.float32, d.int64, d.int32, d.bool, d.string,
        d.event_list_float64, d.event_list_float32, d.event_list_int64,
        d.event_list_int32, d.DataArray
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


def _event_list_dtype_lut():
    from .._scipp.core import dtype as d
    import numpy as np
    dtypes = [
        d.event_list_float64, d.event_list_float32, d.event_list_int64,
        d.event_list_int32
    ]
    numpy_dtypes = [np.float64, np.float32, np.int64, np.int32]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, numpy_dtypes))


class NumpyDataIO():
    @staticmethod
    def write(group, data):
        dset = group.create_dataset('values', data=data.values)
        if data.variances is not None:
            variances = group.create_dataset('variances', data=data.variances)
            dset.attrs['variances'] = variances.ref
        return dset

    @staticmethod
    def read(group, data):
        group['values'].read_direct(data.values)
        if 'variances' in group:
            group['variances'].read_direct(data.variances)


class ScippDataIO():
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


class EventListDataIO():
    @staticmethod
    def write(group, data):
        if len(data.shape) == 0:
            dset = group.create_dataset('values', data=data.values)
        else:
            import h5py
            dt = h5py.vlen_dtype(_event_list_dtype_lut()[str(data.dtype)])
            dset = group.create_dataset('values', shape=data.shape, dtype=dt)
            for i in range(len(data.values)):
                dset[i] = data.values[i]
        return dset

    @staticmethod
    def read(group, data):
        values = group['values']
        if len(data.shape) == 0:
            data.values = values
        else:
            for i in range(len(data.values)):
                data.values[i] = values[i]


class StringDataIO():
    @staticmethod
    def write(group, data):
        import h5py
        dt = h5py.string_dtype(encoding='utf-8')
        if len(data.shape) == 0:
            dset = group.create_dataset('values', data=data.value, dtype=dt)
        else:
            dset = group.create_dataset('values', shape=data.shape, dtype=dt)
            for i in range(len(data.values)):
                dset[i] = data.values[i]
        return dset

    @staticmethod
    def read(group, data):
        values = group['values']
        if len(data.shape) == 0:
            data.value = str(values[...])
        else:
            for i in range(len(data.values)):
                data.values[i] = values[i]


def _check_scipp_version(group):
    if 'scipp-version' not in group.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file/group written by scipp.")


def _data_handler_lut():
    from .._scipp.core import dtype as d
    handler = {}
    for dtype in [d.float64, d.float32, d.int64, d.int32, d.bool]:
        handler[str(dtype)] = NumpyDataIO
    for dtype in [d.DataArray]:
        handler[str(dtype)] = ScippDataIO
    for dtype in _event_list_dtype_lut().keys():
        handler[dtype] = EventListDataIO
    for dtype in [d.string]:
        handler[str(dtype)] = StringDataIO
    return handler


class VariableIO:
    _dtypes = _dtype_lut()
    _units = _unit_lut()
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
        from .._scipp import __version__
        group.attrs['scipp-version'] = __version__
        group.attrs['scipp-type'] = 'Variable'
        dset = cls._write_data(group, var)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        dset.attrs['unit'] = str(var.unit)
        return group

    @classmethod
    def read(cls, group):
        _check_scipp_version(group)
        from .._scipp import core as sc
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = cls._dtypes[values.attrs['dtype']]
        contents['unit'] = cls._units[values.attrs['unit']]
        contents['variances'] = 'variances' in group
        var = sc.Variable(**contents)
        cls._read_data(group, var)
        return var


class DataArrayIO:
    @staticmethod
    def write(group, data):
        from .._scipp import __version__
        group.attrs['scipp-version'] = __version__
        group.attrs['scipp-type'] = 'DataArray'
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError(
                "Writing realigned data is not implemented yet.")
        VariableIO.write(group.create_group('data'), var=data.data)
        views = [data.coords, data.masks]
        aligned = data.aligned_coords.keys()
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks'], views):
            subgroup = group.create_group(view_name)
            for name in view:
                g = VariableIO.write(group=subgroup.create_group(str(name)),
                                     var=view[name])
                if view_name == 'coords':
                    g.attrs['aligned'] = name in aligned

    @staticmethod
    def read(group):
        _check_scipp_version(group)
        from .._scipp import core as sc
        contents = dict()
        contents['name'] = group.attrs['name']
        contents['data'] = VariableIO.read(group['data'])
        contents['unaligned_coords'] = dict()
        for category in ['coords', 'masks']:
            contents[category] = dict()
            for name in group[category]:
                g = group[category][name]
                c = category
                if category == 'coords' and not g.attrs.get('aligned', True):
                    c = 'unaligned_coords'
                contents[c][name] = VariableIO.read(g)
        return sc.DataArray(**contents)


class DatasetIO:
    @staticmethod
    def write(group, data):
        from .._scipp import __version__
        group.attrs['scipp-version'] = __version__
        group.attrs['scipp-type'] = 'Dataset'
        for name in data:
            HDF5IO.write(group.create_group(name), data[name])

    @staticmethod
    def read(group):
        _check_scipp_version(group)
        from .._scipp import core as sc
        d = sc.Dataset()
        for name in group:
            d[name] = HDF5IO.read(group[name])
        return d


class HDF5IO:
    _handlers = dict(
        zip(['Variable', 'DataArray', 'Dataset'],
            [VariableIO, DataArrayIO, DatasetIO]))

    @classmethod
    def write(cls, group, data):
        name = data.__class__.__name__.replace('View', '')
        return cls._handlers[name].write(group, data)

    @classmethod
    def read(cls, group):
        return cls._handlers[group.attrs['scipp-type']].read(group)


def to_hdf5(obj, filename):
    import h5py
    with h5py.File(filename, 'w') as f:
        HDF5IO.write(f, obj)


def open_hdf5(filename):
    import h5py
    with h5py.File(filename, 'r') as f:
        return HDF5IO.read(f)
