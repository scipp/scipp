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


class HDF5NumpyHandler():
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


class HDF5ScippHandler():
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


class HDF5EventListHandler():
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


class HDF5StringHandler():
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


def _data_handler_lut():
    from .._scipp.core import dtype as d
    handler = {}
    for dtype in [d.float64, d.float32, d.int64, d.int32, d.bool]:
        handler[str(dtype)] = HDF5NumpyHandler
    for dtype in [d.DataArray]:
        handler[str(dtype)] = HDF5ScippHandler
    for dtype in _event_list_dtype_lut().keys():
        handler[dtype] = HDF5EventListHandler
    for dtype in [d.string]:
        handler[str(dtype)] = HDF5StringHandler
    return handler


class VariableIO:
    @staticmethod
    def write(group, var):
        if var.dtype not in HDF5IO._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            print(f'Writing with dtype={var.dtype} not implemented, skipping.')
            return
        from .._scipp import __version__
        group.attrs['scipp-version'] = __version__
        group.attrs['scipp-type'] = 'DataArray'
        dset = HDF5IO._write_data(group, var)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        dset.attrs['unit'] = str(var.unit)
        return group

    @staticmethod
    def read(group):
        _check_scipp_version(group)
        from .._scipp import core as sc
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = HDF5IO._read_dtype(values)
        contents['unit'] = HDF5IO._read_unit(values)
        contents['variances'] = 'variances' in group
        var = sc.Variable(**contents)
        HDF5IO._read_data(group, var)
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
    pass


def _handler_lut():
    return dict(
        zip(['Variable', 'DataArray', 'Dataset'],
            [VariableIO, DataArrayIO, DatasetIO]))


def _check_scipp_version(group):
    if 'scipp-version' not in group.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file/group written by scipp.")


class HDF5IO:
    _units = _unit_lut()
    _dtypes = _dtype_lut()
    _data_handlers = _data_handler_lut()
    _handlers = _handler_lut()

    @staticmethod
    def _read_unit(dset):
        return HDF5IO._units[dset.attrs['unit']]

    @staticmethod
    def _read_dtype(dset):
        return HDF5IO._dtypes[dset.attrs['dtype']]

    @staticmethod
    def _write_data(group, data):
        return HDF5IO._data_handlers[str(data.dtype)].write(group, data)

    @staticmethod
    def _read_data(group, data):
        return HDF5IO._data_handlers[str(data.dtype)].read(group, data)

    @staticmethod
    def write(group, data):
        return HDF5IO._handlers[data.__class__.__name__].write(group, data)

    @staticmethod
    def read(group):
        return HDF5IO._handlers[group.attrs['scipp-type']].read(group)


def data_array_to_hdf5(self, filename):
    import h5py
    with h5py.File(filename, 'w') as f:
        HDF5IO._handlers['DataArray'].write(f, self)
        #io._write_data_array(f, self)


def open_hdf5(filename):
    import h5py
    with h5py.File(filename, 'r') as f:
        return HDF5IO.read(f)
