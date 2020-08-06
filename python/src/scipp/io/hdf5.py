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
    dtypes = [d.float64, d.float32, d.int64, d.int32, d.bool]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


class HDF5IO:
    _units = _unit_lut()
    _dtypes = _dtype_lut()

    def _read_unit(self, dset):
        return self._units[dset.attrs['unit']]

    def _read_dtype(self, dset):
        return self._dtypes[dset.attrs['dtype']]

    def _write_array(self, group, name, array):
        import h5py
        # Support for strings not enabled yet (see _dtype_lut), but something
        # like this will work (if generalized to arrays of strings). Will need
        # to also solve reading, e.g., but iterating all elements by hand.
        if isinstance(array, str):
            return group.create_dataset(name,
                                        data=array,
                                        dtype=h5py.string_dtype())
        return group.create_dataset(name, data=array)

    def _write_variable(self, group, var, name):
        if var.dtype not in self._dtypes.values():
            # In practice this may make the file unreadable, e.g., if values
            # have unsupported dtype.
            print(f'Writing with dtype={var.dtype} not implemented, skipping.')
            return
        group = group.create_group(name)
        dset = self._write_array(group, 'values', var.values)
        dset.attrs['dims'] = [str(dim) for dim in var.dims]
        dset.attrs['shape'] = var.shape
        dset.attrs['dtype'] = str(var.dtype)
        dset.attrs['unit'] = str(var.unit)
        if var.variances is not None:
            variances = self._write_array(group, 'variances', var.variances)
            dset.attrs['variances'] = variances.ref

    def _read_variable(self, group):
        from .._scipp import core as sc
        values = group['values']
        contents = {key: values.attrs[key] for key in ['dims', 'shape']}
        contents['dtype'] = self._read_dtype(values)
        contents['unit'] = self._read_unit(values)
        contents['variances'] = 'variances' in group
        var = sc.Variable(**contents)
        values.read_direct(var.values)
        if 'variances' in group:
            group['variances'].read_direct(var.variances)
        return var

    def _write_data_array(self, group, data):
        import scipp as sc
        group.attrs['scipp-version'] = sc.__version__
        group.attrs['type'] = 'DataArray'
        group.attrs['name'] = data.name
        if data.data is None:
            raise RuntimeError(
                "Writing realigned data is not implemented yet.")
        self._write_variable(group, var=data.data, name='data')
        views = [data.aligned_coords, data.masks, data.unaligned_coords]
        for view_name, view in zip(['coords', 'masks', 'unaligned_coords'],
                                   views):
            subgroup = group.create_group(view_name)
            for name in view:
                self._write_variable(group=subgroup,
                                     var=view[name],
                                     name=str(name))

    def _read_data_array(self, group):
        from .._scipp import core as sc
        contents = dict()
        contents['name'] = group.attrs['name']
        contents['data'] = self._read_variable(group['data'])
        for category in ['coords', 'masks', 'unaligned_coords']:
            contents[category] = dict()
            for name in group[category]:
                contents[category][name] = self._read_variable(
                    group[category][name])
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
