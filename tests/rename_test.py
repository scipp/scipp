# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import scipp as sc
import pytest


def make_dataarray(dim1, dim2) -> sc.DataArray:
    from numpy.random import default_rng
    rng = default_rng(seed=1234)
    da = sc.DataArray(sc.array(dims=[dim1, dim2], values=rng.random((100, 100))))
    da.coords[dim1] = sc.linspace(dim1, 0.0, 1.0, num=100, unit='mm')
    da.coords[dim2] = sc.linspace(dim2, 0.0, 5.0, num=100, unit='mm')
    return da


def test_rename_dims():
    da = make_dataarray('x', 'y')
    original = da.copy()
    renamed = da.rename_dims({'y': 'z'})
    assert sc.identical(da, original)
    renamed.coords['z'] = renamed.coords['y']
    del renamed.coords['y']
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename_dims({'x': 'y', 'z': 'x'})
    renamed.coords['y'] = renamed.coords['x']
    renamed.coords['x'] = renamed.coords['z']
    del renamed.coords['z']
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename():
    da = make_dataarray('x', 'y')
    original = da.copy()
    renamed = da.rename({'y': 'z'})
    assert sc.identical(da, original)
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename({'x': 'y', 'z': 'x'})
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename_kwargs():
    da = make_dataarray('x', 'y')
    renamed = da.rename(y='z')
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename(x='y', z='x')
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename_with_attr():
    da = make_dataarray('x', 'y')
    da.attrs['y'] = da.coords.pop('y')
    renamed = da.rename({'y': 'z'})
    expected = make_dataarray('x', 'z')
    expected.attrs['z'] = expected.coords.pop('z')
    assert sc.identical(renamed, expected)


def test_rename_fails_when_coord_already_exists():
    da = make_dataarray('x', 'y')
    da.coords['z'] = da.coords['x'].copy()
    with pytest.raises(sc.CoordError):
        da.rename({'x': 'z'})


def test_rename_fails_when_attr_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['y'] = da.coords.pop('y')
    da.attrs['z'] = da.attrs['y'].copy()
    with pytest.raises(sc.CoordError):
        da.rename({'y': 'z'})


def test_rename_fails_when_attr_with_same_name_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['meta'] = sc.scalar(5)
    with pytest.raises(sc.CoordError):
        da.rename({'x': 'meta'})


def test_rename_fails_when_coord_with_same_name_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['aux'] = sc.scalar(5)
    da.attrs['y'] = da.coords.pop('y')
    with pytest.raises(sc.CoordError):
        da.rename({'y': 'aux'})
