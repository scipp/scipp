# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
"""
Convert pre-0.12 HDF5 files to 0.12-compatible files.

Changes are:

* Unit of values of `begin` and `end` indices for binned data removed
"""

import sys
from shutil import copyfile

import h5py


def migrate_object(name, obj):
    if obj.attrs.get('dtype') in ('VariableView', 'DataArrayView', 'DatasetView'):
        del obj['begin/values'].attrs['unit']
        del obj['end/values'].attrs['unit']


def migrate_file(filename):
    with h5py.File(filename, 'r+') as f:
        f.visititems(migrate_object)


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print('Usage: python scipp-0.12-hdf5-files.py filename1 [filename2...]')
    for filename in sys.argv[1:]:
        backup = f'{filename}.pre-0.12'
        print(f'Backing up original file to {backup}')
        copyfile(filename, backup)
        migrate_file(filename)
