# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import scipp as sc
from scipp.core.domains import find_domains
import pytest

x = sc.arange(dim='x', start=0, stop=10)


def test_find_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})

    expected = sc.DataArray(data=sc.array(dims=['x'], values=[1]),
                            coords={'x': sc.array(dims=['x'], values=[0, 9])})
    assert sc.identical(find_domains(da), expected)
    assert sc.identical(find_domains(da, 'x'), expected)
    with pytest.raises(sc.NotFoundError):
        find_domains(da, 'y')
