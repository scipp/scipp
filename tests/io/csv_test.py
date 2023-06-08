# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from io import StringIO

import pytest

import scipp as sc
from scipp.testing import assert_identical


@pytest.mark.parametrize('sep', ('\t', ','))
def test_load_csv_dataset_default(sep):
    csv = '''x\ty\tz
1\t2\t3
4\t5\t6
7\t8\t9
10\t11\t12'''.replace(
        '\t', sep
    )

    loaded = sc.io.load_csv(StringIO(csv), sep=sep)
    expected = sc.Dataset(
        {
            'x': sc.array(dims=['row'], values=[1, 4, 7, 10]),
            'y': sc.array(dims=['row'], values=[2, 5, 8, 11]),
            'z': sc.array(dims=['row'], values=[3, 6, 9, 12]),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1, 2, 3])},
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_default_sep():
    csv = '''x\ty\tz
1\t2\t3
4\t5\t6
7\t8\t9
10\t11\t12'''

    loaded = sc.io.load_csv(StringIO(csv))
    expected = sc.Dataset(
        {
            'x': sc.array(dims=['row'], values=[1, 4, 7, 10]),
            'y': sc.array(dims=['row'], values=[2, 5, 8, 11]),
            'z': sc.array(dims=['row'], values=[3, 6, 9, 12]),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1, 2, 3])},
    )
    assert_identical(loaded, expected)


@pytest.mark.parametrize('data_columns', ('xyz', ['xyz']))
def test_load_csv_dataset_select_single_data(data_columns):
    csv = '''abc\txyz\tfoo
1.2\t3.4\t5.6
0.8\t0.6\t0.4'''

    loaded = sc.io.load_csv(StringIO(csv), data_columns=data_columns)
    expected = sc.Dataset(
        {
            'xyz': sc.array(dims=['row'], values=[3.4, 0.6]),
        },
        coords={
            'row': sc.array(dims=['row'], values=[0, 1]),
            'abc': sc.array(dims=['row'], values=[1.2, 0.8]),
            'foo': sc.array(dims=['row'], values=[5.6, 0.4]),
        },
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_select_multiple_data():
    csv = '''abc\txyz\tfoo
1.2\t3.4\t5.6
0.8\t0.6\t0.4'''

    loaded = sc.io.load_csv(StringIO(csv), data_columns=['xyz', 'abc'])
    expected = sc.Dataset(
        {
            'abc': sc.array(dims=['row'], values=[1.2, 0.8]),
            'xyz': sc.array(dims=['row'], values=[3.4, 0.6]),
        },
        coords={
            'row': sc.array(dims=['row'], values=[0, 1]),
            'foo': sc.array(dims=['row'], values=[5.6, 0.4]),
        },
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_select_no_data():
    csv = '''abc\txyz\tfoo
1.2\t3.4\t5.6
0.8\t0.6\t0.4'''

    loaded = sc.io.load_csv(StringIO(csv), data_columns=[])
    expected = sc.Dataset(
        {},
        coords={
            'row': sc.array(dims=['row'], values=[0, 1]),
            'abc': sc.array(dims=['row'], values=[1.2, 0.8]),
            'xyz': sc.array(dims=['row'], values=[3.4, 0.6]),
            'foo': sc.array(dims=['row'], values=[5.6, 0.4]),
        },
    )
    assert_identical(loaded, expected)


def test_load_csv_dataset_select_undefined_data_raises():
    csv = '''abc\txyz\tfoo
1.2\t3.4\t5.6
0.8\t0.6\t0.4'''

    with pytest.raises(KeyError):
        _ = sc.io.load_csv(StringIO(csv), data_columns=['does-not-exist'])


def test_load_csv_dataset_without_index():
    csv = '''abc\txyz
1.2\t3.4
0.8\t0.6'''

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
    csv = '''abc [m]\t [kg/s]\t foo
1.2\t3.4\t5.6
0.8\t0.6\t0.4'''

    loaded = sc.io.load_csv(StringIO(csv), head_parser="bracket")
    expected = sc.Dataset(
        {
            'abc': sc.array(dims=['row'], values=[1.2, 0.8], unit='m'),
            '': sc.array(dims=['row'], values=[3.4, 0.6], unit='kg/s'),
            ' foo': sc.array(dims=['row'], values=[5.6, 0.4], unit='one'),
        },
        coords={'row': sc.array(dims=['row'], values=[0, 1])},
    )
    assert_identical(loaded, expected)
