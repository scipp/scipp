# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock


def _unit_lut():
    from .._scipp.core import units as u
    units = [u.one, u.m, u.kg, u.us, u.angstrom]
    names = [str(unit) for unit in units]
    return dict(zip(names, units))


class HDF5IO:
    _units = _unit_lut()

    def _read_unit(self, dset):
        return self._units[dset.attrs['unit']]

    def _write_variable(self, group, var, name):
        group = group.create_group(name)
        dset = group.create_dataset('values', data=var.values)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['dtype'] = str(var.values.dtype)
        dset.attrs['unit'] = str(var.unit)
        if var.variances is not None:
            variances = group.create_dataset('variances', data=var.variances)
            dset.attrs['variances'] = variances.ref

    def _read_variable(self, group):
        from .._scipp import core as sc
        import numpy as np
        values = group['values']
        dtype = values.attrs['dtype']
        contents = dict()
        # For some reason we cannot pass the dataset directly to scipp, and
        # we also need to manually override the dtype. Is h5py using a
        # different float type?
        contents['values'] = np.array(values, dtype=dtype, copy=False)
        contents['dims'] = values.attrs['dims']
        contents['unit'] = self._read_unit(values)
        if 'variances' in group:
            contents['variances'] = np.array(group['variances'],
                                             dtype=dtype,
                                             copy=False)
        return sc.Variable(**contents)

    def _write_data_array(self, group, data):
        import scipp as sc
        group.attrs['scipp-version'] = sc.__version__
        group.attrs['type'] = 'DataArray'
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError(
                "Writing realigned data is not implemented yet.")
        self._write_variable(group, var=data.data, name='data')
        coords = group.create_group('coords')
        for name in data.coords:
            self._write_variable(group=coords,
                                 var=data.coords[name],
                                 name=str(name))
        masks = group.create_group('masks')
        for name in data.masks:
            self._write_variable(group=masks, var=data.masks[name], name=name)
        attrs = group.create_group('attrs')
        for name in data.attrs:
            self._write_variable(group=attrs, var=data.attrs[name], name=name)

    def _read_data_array(self, group):
        from .._scipp import core as sc
        contents = dict()
        contents['data'] = self._read_variable(group['data'])
        for category in ['coords', 'masks', 'attrs']:
            contents[category] = dict()
            for name in group[category]:
                contents[category][name] = self._read_variable(
                    group[category][name])
        return sc.DataArray(**contents)


def data_array_to_hdf5(self, filename):
    import h5py
    f = h5py.File(filename, 'w')
    io = HDF5IO()
    io._write_data_array(f, self)


def open_hdf5(filename):
    import h5py
    f = h5py.File(filename, 'r')
    io = HDF5IO()
    if not 'scipp-version' in f.attrs:
        raise RuntimeError(
            "This does not look like an HDF5 file written by scipp.")
    if f.attrs['type'] == 'DataArray':
        return io._read_data_array(f)
    raise RuntimeError(f"Unknown data type {f.attrs['type']})")
