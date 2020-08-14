import scipp as sc
from scipp.slice import slice
import numpy as np

working_unit = sc.units.dimensionless


def _make_1d_data_array(begin, end, dim_name='x', bin_edges=False):
    if bin_edges:
        data = sc.Variable([dim_name], values=np.arange(end - begin - 1))
    else:
        data = sc.Variable([dim_name], values=np.arange(end - begin))
    x = sc.Variable([dim_name], values=np.arange(begin, end))
    return sc.DataArray(data=data, coords={dim_name: x})


def test_slice_range_on_point_coords_1D():
    #    Data Values           [0.0][1.0] ... [8.0][9.0]
    #    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

    da = _make_1d_data_array(begin=3.0,
                             end=13.0,
                             dim_name='x',
                             bin_edges=False)
    # Test start on left boundary (closed on left), so includes boundary
    out = slice(da, 'x', start=3.0 * working_unit, end=4.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    assert sc.is_equal(out.data, da['x', 0:1].data)
    # Test start out of bounds on left truncated
    out = slice(da, 'x', start=2.0 * working_unit, end=4.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test end on right boundary (open on right), so does not include boundary
    out = slice(da, 'x', start=11.0 * working_unit, end=12.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', -2:-1].coords['x'])
    # Test start out of bounds on right truncated
    out = slice(da, 'x', start=11.0 * working_unit, end=13.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', -2:].coords['x'])


def test_slice_range_on_edge_coords_1D():
    #    Data Values            [0.0] ...       [9.0]
    #    Coord Values (edges) [3.0][4.0] ... [11.0][12.0]
    da = _make_1d_data_array(begin=3.0, end=13.0, dim_name='x', bin_edges=True)
    print(da)
    # Test start on left boundary (closed on left), so includes boundary
    out = slice(da, 'x', start=3.0 * working_unit, end=4.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    assert sc.is_equal(out.data, da['x', 0:1].data)
    # Test slicing with range boundary inside edge, same result as above expected
    out = slice(da, 'x', start=3.1 * working_unit, end=4.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test slicing with range lower boundary on upper edge of bin (open on right test)
    out = slice(da, 'x', start=4.0 * working_unit, end=6.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', 1:3].coords['x'])
    # Test end on right boundary (open on right), so does not include boundary
    out = slice(da, 'x', start=11.0 * working_unit, end=12.0 * working_unit)
    assert sc.is_equal(out.coords['x'], da['x', -1:].coords['x'])  #


def test_slice_range_on_point_coords_2D():
    data = sc.Variable(['y', 'x'], values=np.arange(100).reshape(5, 20))
    x = sc.Variable(['x'], values=np.arange(-10.0, 10.0))
    y = sc.Variable(['y'], values=np.arange(-2.5, 2.5))
    da = sc.DataArray(data=data, coords={'x': x, 'y': y})
    out = slice(da, 'x', start=-10.0 * working_unit, end=-9.0 * working_unit)
    #assert sc.is_equal
