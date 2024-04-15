# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
"""
Convert pre-0.11 HDF5 files to 0.11-compatible files.

Changes are:

* Rename dtype vector_3_float64 to vector3
* Rename dtype matrix_3_float64 to linear_transform3
"""

import sys
from shutil import copyfile

import h5py


def migrate_object(name, obj):
    if 'dtype' in obj.attrs:
        mapping = {
            'vector_3_float64': 'vector3',
            'matrix_3_float64': 'linear_transform3',
        }
        for old, new in mapping.items():
            if obj.attrs['dtype'] == old:
                obj.attrs['dtype'] = new
                print(f'Changing dtype attr of {name} from {old} to {new}')


def migrate_file(filename):
    with h5py.File(filename, 'r+') as f:
        f.visititems(migrate_object)


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print('Usage: python scipp-0.11-hdf5-files.py filename1 [filename2...]')
    for filename in sys.argv[1:]:
        backup = f'{filename}.pre-0.11'
        print(f'Backing up original file to {backup}')
        copyfile(filename, backup)
        migrate_file(filename)
