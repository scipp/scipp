# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from io import StringIO

import pytest

import scipp as sc
from scipp.testing import assert_identical


def test_load_csv_dataset_default_sep():
    csv = '''x,y,z
1,2,3
4,5,6
7,8,9
10,11,12'''

    loaded = sc.io.load_csv(StringIO(csv))
    expected = sc.Dataset(
        {
            'x': sc.array(dims=['row'], values=[1, 4, 7, 10]),
            'y': sc.array(dims=['row'], values=[2, 5, 8, 11]),
            'z': sc.array(dims=['row'], values=[3, 6, 9, 12]),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1, 2, 3], unit=None)},
    )
    assert_identical(loaded, expected)


@pytest.mark.parametrize('sep', ('\t', '/'))
def test_load_csv_dataset_choose_separator(sep):
    csv = '''x,y,z
1,2,3
4,5,6
7,8,9
10,11,12'''.replace(
        ',', sep
    )

    loaded = sc.io.load_csv(StringIO(csv), sep=sep)
    expected = sc.Dataset(
        {
            'x': sc.array(dims=['row'], values=[1, 4, 7, 10]),
            'y': sc.array(dims=['row'], values=[2, 5, 8, 11]),
            'z': sc.array(dims=['row'], values=[3, 6, 9, 12]),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1, 2, 3], unit=None)},
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_select_data():
    csv = '''abc,xyz,foo
1.2,3.4,5.6
0.8,0.6,0.4'''

    loaded = sc.io.load_csv(StringIO(csv), data_columns=['xyz', 'abc'])
    expected = sc.Dataset(
        {
            'abc': sc.array(dims=['row'], values=[1.2, 0.8]),
            'xyz': sc.array(dims=['row'], values=[3.4, 0.6]),
        },
        coords={
            'row': sc.array(dims=['row'], values=[0, 1], unit=None),
            'foo': sc.array(dims=['row'], values=[5.6, 0.4]),
        },
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_without_index():
    csv = '''abc,xyz
1.2,3.4
0.8,0.6'''

    loaded = sc.io.load_csv(StringIO(csv), include_index=False)
    expected = sc.Dataset(
        {
            'abc': sc.array(dims=['row'], values=[1.2, 0.8]),
            'xyz': sc.array(dims=['row'], values=[3.4, 0.6]),
        },
        coords={},
    )
    assert_identical(loaded, expected)


def test_load_csv_parse_units():
    csv = '''abc [m], [kg/s], foo
1.2,3.4,5.6
0.8,0.6,0.4'''

    loaded = sc.io.load_csv(StringIO(csv), header_parser="bracket")
    expected = sc.Dataset(
        {
            'abc': sc.array(dims=['row'], values=[1.2, 0.8], unit='m'),
            '': sc.array(dims=['row'], values=[3.4, 0.6], unit='kg/s'),
            ' foo': sc.array(dims=['row'], values=[5.6, 0.4], unit=None),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1], unit=None)},
    )
    assert_identical(loaded, expected)


def test_load_csv_forwards_kwargs_to_pandas():
    csv = '''abc,xyz,foo
1.2,3.4,5.6
0.8,0.6,0.4'''

    loaded = sc.io.load_csv(StringIO(csv), names=['x', 'y', 'z'], header=0)
    expected = sc.Dataset(
        {
            'x': sc.array(dims=['row'], values=[1.2, 0.8]),
            'y': sc.array(dims=['row'], values=[3.4, 0.6]),
            'z': sc.array(dims=['row'], values=[5.6, 0.4]),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1], unit=None)},
    )
    assert_identical(loaded, expected)
