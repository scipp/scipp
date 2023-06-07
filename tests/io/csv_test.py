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
