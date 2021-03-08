import numpy as np
import scipp as sc
import scipp._utils as su
import pytest


def test_wont_match_when_meta_size_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point)
    with pytest.raises(RuntimeError):
        su.is_near(a, b, rtol=0, atol=1.0)
    # Raise nothing if we are ignoring differing parts
    su.is_near(a, b, rtol=0, atol=1.0, include_attrs=False)


def test_wont_match_when_meta_keys_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point, attrs={'y': point})
    with pytest.raises(RuntimeError):
        su.is_near(a, b, rtol=0, atol=1.0)
    # Raise nothing if we are ignoring differing parts
    su.is_near(a, b, rtol=0, atol=1.0, include_attrs=False)


def test_wont_match_when_meta_sizes_unequal():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, attrs={'x': point})
    b = sc.DataArray(data=point,
                     attrs={'x': sc.array(dims=['x'], values=np.arange(2))})
    with pytest.raises(RuntimeError):
        su.is_near(a, b, rtol=0, atol=1.0)
    # Raise nothing if we are ignoring differing parts
    su.is_near(a, b, rtol=0, atol=1.0, include_attrs=False)


def test_wont_match_when_non_finite_sizes_unequal():
    x = sc.array(dims=['x'], values=np.array([1.0, 2.0, np.nan]))
    xx = sc.array(dims=['x'], values=np.array([1.0, 2.0, 3.0]))
    a = sc.DataArray(data=x, attrs={'x': x})
    b = sc.DataArray(data=x, attrs={'x': xx})
    with pytest.raises(RuntimeError):
        su.is_near(a, b, rtol=0, atol=1.0)
    # Raise nothing if we are ignoring differing parts
    su.is_near(a, b, rtol=0, atol=1.0, include_attrs=False)
    # Having nans is fine as long as equal_nan flag passed
    su.is_near(a, a, rtol=0, atol=1.0, equal_nan=True)


def test_data_scalar_no_coords():
    a = sc.DataArray(data=sc.scalar(value=1.0))
    assert su.is_near(a, a, rtol=0, atol=1e-14)
    b = a + 1.0 * sc.units.one
    assert su.is_near(a, b, rtol=0.0, atol=1.0)
    assert su.is_near(a, b, rtol=1.0, atol=0.0)
    assert not su.is_near(a, b, rtol=0, atol=0.9999)


def test_data_scalar_no_coords_no_data():
    a = sc.DataArray(data=sc.scalar(value=1))
    b = a.copy()
    assert su.is_near(a, b, rtol=0, atol=1e-14,
                      include_data=False)  # Should compare equal


def test_scalar_with_coord():
    point = sc.scalar(value=1.0)
    a = sc.DataArray(data=point, coords={'x': point})
    assert su.is_near(a, a, rtol=0.0, atol=1e-14)
    b = sc.DataArray(data=point, coords={'x': point + point})
    assert su.is_near(a, b, rtol=0.0, atol=1.0)
    assert not su.is_near(a, b, rtol=0.0, atol=0.9999)


def test_with_many_coords():
    x = sc.array(dims=['x'], values=np.arange(10.0))
    xx = sc.array(dims=['x'], values=np.arange(1, 11.0))
    a = sc.DataArray(data=x, coords={'a': x, 'b': x})
    b = sc.DataArray(data=x, coords={'a': x, 'b': xx})
    assert su.is_near(a, a, rtol=0.0, atol=1e-14)
    assert su.is_near(a, b, rtol=0.0, atol=1.0)
    assert not su.is_near(a, b, rtol=0.0, atol=0.9999)


def test_with_many_coords_and_attrs():
    x = sc.array(dims=['x'], values=np.arange(10.0))
    xx = sc.array(dims=['x'], values=np.arange(1, 11.0))
    a = sc.DataArray(data=x, coords={'a': x, 'b': x}, attrs={'c': x, 'd': x})
    b = sc.DataArray(data=x, coords={'a': x, 'b': x}, attrs={'c': x, 'd': xx})
    assert su.is_near(a, a, rtol=0.0, atol=1e-14)
    assert su.is_near(a, b, rtol=0.0, atol=1.0)
    assert not su.is_near(a, b, rtol=0.0, atol=0.9999)
    # Now disable attrs matching (should be near)
    assert su.is_near(a, b, rtol=0.0, atol=0.9999, include_attrs=False)
