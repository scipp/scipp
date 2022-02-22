# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
"""
Convert pre-0.13 HDF5 files to 0.13-compatible files.

Changes are:

* Names of coords, attrs, and masks are encoded via attributes.
* Names of data arrays are stored in attributes called 'scipp-name'
  instead of just 'name'.
"""
import h5py
import sys
from shutil import copyfile


def migrate_variable(group):
    vals = group['values']
    if isinstance(vals, h5py.Group):
        if vals.attrs['dtype'] in ('DataArrayView', 'DatasetView'):
            migrate_group(vals['data'])
        else:
            migrate_group(vals)


def migrate_meta_data(group):
    for i, name in enumerate(group):
        new_name = f'elem_{i}'
        group[new_name] = group.pop(name)
        group[new_name].attrs['scipp-name'] = name
        migrate_variable(group[new_name])


def migrate_data_array(group):
    group.attrs['scipp-name'] = group.attrs.pop('name')
    migrate_variable(group['data'])
    for category in ('coords', 'masks', 'attrs'):
        migrate_meta_data(group[category])


def migrate_dataset(group):
    for i, name in enumerate(list(group)):
        new_name = f'elem_{i}'
        group[new_name] = group.pop(name)
        migrate_data_array(group[new_name])


def migrate_group(group):
    if group.attrs['scipp-type'] == 'Dataset':
        migrate_dataset(group)
    if group.attrs['scipp-type'] == 'DataArray':
        migrate_data_array(group)


def migrate_file(filename):
    with h5py.File(filename, 'r+') as f:
        migrate_group(f)


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print('Usage: python scipp-0.13-hdf5-files.py filename1 [filename2...]')
    for filename in sys.argv[1:]:
        backup = f'{filename}.pre-0.13'
        print(f'Backing up original file to {backup}')
        copyfile(filename, backup)
        migrate_file(filename)
