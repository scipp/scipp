import numpy as np
import pytest

import scipp as sc
import scipp.utils as su


def test_wont_match_when_meta_size_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point)
    assert not su.isnear(a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one)
    # ignore attributes, should give the same result
    assert su.isnear(
        a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one, include_attrs=False
    )


def test_wont_match_when_meta_keys_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point, attrs={'y': point})
    with pytest.raises(KeyError):
        su.isnear(a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one)
    # Raise nothing if we are ignoring differing parts
    su.isnear(a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one, include_attrs=False)


def test_wont_match_when_meta_sizes_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point, attrs={'x': sc.array(dims=['x'], values=np.arange(2))})
    with pytest.raises(sc.CoordError):
        su.isnear(a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one)
    # Raise nothing if we are ignoring differing parts
    su.isnear(a, b, rtol=0 * sc.units.one, atol=1.0 * sc.units.one, include_attrs=False)


def test_data_scalar_no_coords():
    a = sc.DataArray(data=sc.scalar(value=1.0))
    assert su.isnear(a, a, rtol=0 * sc.units.one, atol=1e-14 * sc.units.one)
    b = a + 1.0 * sc.units.one
    assert su.isnear(a, b, rtol=0.0 * sc.units.one, atol=1.0 * sc.units.one)
    assert su.isnear(a, b, rtol=1.0 * sc.units.one, atol=0.0 * sc.units.one)
    assert not su.isnear(a, b, rtol=0 * sc.units.one, atol=0.9999 * sc.units.one)


def test_data_scalar_no_coords_no_data():
    a = sc.DataArray(data=sc.scalar(value=1))
    b = a.copy()
    assert su.isnear(
        a, b, rtol=0 * sc.units.one, atol=1e-14 * sc.units.one, include_data=False
    )  # Should compare equal


def test_scalar_with_coord():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, coords={'x': point})
    assert su.isnear(a, a, rtol=0.0 * sc.units.one, atol=1e-14 * sc.units.one)
    b = sc.DataArray(data=point, coords={'x': point + point})
    assert su.isnear(a, b, rtol=0.0 * sc.units.one, atol=1.0 * sc.units.one)
    assert not su.isnear(a, b, rtol=0.0 * sc.units.one, atol=0.9999 * sc.units.one)


def test_with_many_coords():
    x = sc.array(dims=['x'], values=np.arange(10.0))
    xx = sc.array(dims=['x'], values=np.arange(1, 11.0))
    a = sc.DataArray(data=x, coords={'a': x, 'b': x})
    b = sc.DataArray(data=x, coords={'a': x, 'b': xx})
    assert su.isnear(a, a, rtol=0.0 * sc.units.one, atol=1e-14 * sc.units.one)
    assert su.isnear(a, b, rtol=0.0 * sc.units.one, atol=1.0 * sc.units.one)
    assert not su.isnear(a, b, rtol=0.0 * sc.units.one, atol=0.9999 * sc.units.one)


def test_with_many_coords_and_attrs():
    x = sc.array(dims=['x'], values=np.arange(10.0))
    xx = sc.array(dims=['x'], values=np.arange(1, 11.0))
    a = sc.DataArray(data=x, coords={'a': x, 'b': x}, attrs={'c': x, 'd': x})
    b = sc.DataArray(data=x, coords={'a': x, 'b': x}, attrs={'c': x, 'd': xx})
    assert su.isnear(a, a, rtol=0.0 * sc.units.one, atol=1e-14 * sc.units.one)
    assert su.isnear(a, b, rtol=0.0 * sc.units.one, atol=1.0 * sc.units.one)
    assert not su.isnear(a, b, rtol=0.0 * sc.units.one, atol=0.9999 * sc.units.one)
    # Now disable attrs matching (should be near)
    assert su.isnear(
        a, b, rtol=0.0 * sc.units.one, atol=0.9999 * sc.units.one, include_attrs=False
    )
