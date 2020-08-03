import scipp as sc
from scipp.slice import slice
import numpy as np

def test_support_multidimensional_arrays

def test_slice_range_on_point_coords():
    data = sc.Variable(['x'], values=np.arange(10.0))
    x = sc.Variable(['x'], values=np.arange(3.0, 13.0))
    da = sc.DataArray(data=x, coords={'x': x})
    start = 3.0 * sc.units.dimensionless
    end = 4.0 * sc.units.dimensionless
    out = slice(da, 'x', start=start, end=end)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    assert sc.is_equal(out.data, da['x', 0:1].data)

def test_slice_range_on_edge_coords_points():
    data = sc.Variable(['x'], values=np.arange(10.0))
    x = sc.Variable(['x'], values=np.arange(3.0, 14.0))
    da = sc.DataArray(data=x, coords={'x': x})
    start = 3.0 * sc.units.dimensionless
    end = 4.0 * sc.units.dimensionless
    out = slice(da, 'x', start=start, end=end)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    assert sc.is_equal(out.data, da['x', 0:1].data)
