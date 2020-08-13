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
        d.float64, d.float32, d.int64, d.int32, d.bool, d.string, d.DataArray
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


def _numpy_dtype_list():
    from .._scipp.core import dtype as d
    return [d.float64, d.float32, d.int64, d.int32, d.bool]


def _scipp_dtype_list():
    from .._scipp.core import dtype as d
    return [d.DataArray]


def _write_data_numpy(group, data):
    dset = group.create_dataset('values', data=data.values)
    if data.variances is not None:
        variances = group.create_dataset('variances', data=data.variances)
        dset.attrs['variances'] = variances.ref
    return dset


def _read_data_numpy(group, data):
    group['values'].read_direct(data.values)
    if 'variances' in group:
        group['variances'].read_direct(data.variances)


def _write_data_scipp(group, data):
    values = group.create_group('values')
    io = HDF5IO()
    if len(data.shape) == 0:
        io._write_data_array(values, data.value)
    else:
        for i, item in enumerate(data.values):
            io._write_data_array(values.create_group(f'value-{i}'), item)
    return values


def _read_data_scipp(group, data):
    values = group['values']
    io = HDF5IO()
    if len(data.shape) == 0:
        data.value = io._read_data_array(values)
    else:
        for i in range(len(data.values)):
            data.values[i] = io._read_data_array(values[f'value-{i}'])


class HDF5IO:
    _units = _unit_lut()
    _dtypes = _dtype_lut()
    _numpy_dtypes = _numpy_dtype_list()
    _scipp_dtypes = _scipp_dtype_list()

    def _read_unit(self, dset):
        return self._units[dset.attrs['unit']]

    def _read_dtype(self, dset):
        return self._dtypes[dset.attrs['dtype']]

    def _write_data(self, group, data):
        if data.dtype in self._numpy_dtypes:
            return _write_data_numpy(group, data)
        if data.dtype in self._scipp_dtypes:
            return _write_data_scipp(group, data)
        raise RuntimeError("unsupported dtype")

    def _read_data(self, group, data):
        if data.dtype in self._numpy_dtypes:
            return _read_data_numpy(group, data)
        if data.dtype in self._scipp_dtypes:
            return _read_data_scipp(group, data)
        raise RuntimeError("unsupported dtype")

    def _write_variable(self, group, var, name):
        if var.dtype not in self._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            print(f'Writing with dtype={var.dtype} not implemented, skipping.')
            return
        group = group.create_group(name)
        dset = self._write_data(group, var)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        dset.attrs['unit'] = str(var.unit)
        return group

    def _read_variable(self, group):
        from .._scipp import core as sc
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = self._read_dtype(values)
        contents['unit'] = self._read_unit(values)
        contents['variances'] = 'variances' in group
        var = sc.Variable(**contents)
        self._read_data(group, var)
        return var

    def _write_data_array(self, group, data):
        from .._scipp import __version__
        group.attrs['scipp-version'] = __version__
        group.attrs['type'] = 'DataArray'
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError(
                "Writing realigned data is not implemented yet.")
        self._write_variable(group, var=data.data, name='data')
        views = [data.coords, data.masks]
        aligned = data.aligned_coords.keys()
        # Note that we write aligned and unaligned coords into the same group.
        # Distinction is via an attribute, which is more natural than having
        # 2 separate groups.
        for view_name, view in zip(['coords', 'masks'], views):
            subgroup = group.create_group(view_name)
            for name in view:
                g = self._write_variable(group=subgroup,
                                         var=view[name],
                                         name=str(name))
                if view_name == 'coords':
                    g.attrs['aligned'] = name in aligned

    def _read_data_array(self, group):
        from .._scipp import core as sc
        contents = dict()
        contents['name'] = group.attrs['name']
        contents['data'] = self._read_variable(group['data'])
        contents['unaligned_coords'] = dict()
        for category in ['coords', 'masks']:
            contents[category] = dict()
            for name in group[category]:
                g = group[category][name]
                c = category
                if category == 'coords' and not g.attrs.get('aligned', True):
                    c = 'unaligned_coords'
                contents[c][name] = self._read_variable(g)
        return sc.DataArray(**contents)


def data_array_to_hdf5(self, filename):
    import h5py
    with h5py.File(filename, 'w') as f:
        io = HDF5IO()
        io._write_data_array(f, self)


def open_hdf5(filename):
    import h5py
    with h5py.File(filename, 'r') as f:
        io = HDF5IO()
        if 'scipp-version' not in f.attrs:
            raise RuntimeError(
                "This does not look like an HDF5 file written by scipp.")
        if f.attrs['type'] == 'DataArray':
            return io._read_data_array(f)
        raise RuntimeError(f"Unknown data type {f.attrs['type']})")
