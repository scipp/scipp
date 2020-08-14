import scipp as sc
import numpy as np
import pytest

working_unit = sc.units.dimensionless


def _make_1d_data_array(begin, end, dim_name='x', bin_edges=False):
    if bin_edges:
        data = sc.Variable([dim_name], values=np.arange(end - begin - 1))
    else:
        data = sc.Variable([dim_name], values=np.arange(end - begin))
    x = sc.Variable([dim_name], values=np.arange(begin, end))
    return sc.DataArray(data=data, coords={dim_name: x})


def test_slicing_defaults():
    da = _make_1d_data_array(begin=3.0,
                             end=13.0,
                             dim_name='x',
                             bin_edges=False)
    # test replicate no-effect slicing
    assert sc.is_equal(da, sc.slice(da, 'x', slice(
        None, 13.0 * working_unit)))  # Note closed on left with default start
    assert sc.is_equal(da['x', :-1],
                       sc.slice(da,
                                'x'))  # Note open on right with default end!


def test_slice_range_on_point_coords_1D():
    #    Data Values           [0.0][1.0] ... [8.0][9.0]
    #    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

    da = _make_1d_data_array(begin=3.0,
                             end=13.0,
                             dim_name='x',
                             bin_edges=False)
    # test no-effect slicing
    out = sc.slice(da, 'x', slice(3.0 * working_unit, 13.0 * working_unit))
    assert sc.is_equal(da, out)
    # Test start on left boundary (closed on left), so includes boundary
    out = sc.slice(da, 'x', slice(3.0 * working_unit, 4.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test start out of bounds on left truncated
    out = sc.slice(da, 'x', slice(2.0 * working_unit, 4.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test inner values
    out = sc.slice(da, 'x', slice(3.5 * working_unit, 5.5 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 1:3].coords['x'])
    # Test end on right boundary (open on right), so does not include boundary
    out = sc.slice(da, 'x', slice(11.0 * working_unit, 12.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', -2:-1].coords['x'])
    # Test start out of bounds on right truncated
    out = sc.slice(da, 'x', slice(11.0 * working_unit, 13.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', -2:].coords['x'])


def test_slice_range_on_edge_coords_1D():
    #    Data Values            [0.0] ...       [9.0]
    #    Coord Values (edges) [3.0][4.0] ... [11.0][12.0]
    da = _make_1d_data_array(begin=3.0, end=13.0, dim_name='x', bin_edges=True)
    # test no-effect slicing
    out = sc.slice(da, 'x', slice(3.0 * working_unit, 13.0 * working_unit))
    assert sc.is_equal(da, out)
    # Test start on left boundary (closed on left), so includes boundary
    out = sc.slice(da, 'x', slice(3.0 * working_unit, 4.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test slicing with range boundary inside edge, same result as above expected
    out = sc.slice(da, 'x', slice(3.1 * working_unit, 4.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 0:1].coords['x'])
    # Test slicing with range lower boundary on upper edge of bin (open on right test)
    out = sc.slice(da, 'x', slice(4.0 * working_unit, 6.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', 1:3].coords['x'])
    # Test end on right boundary (open on right), so does not include boundary
    out = sc.slice(da, 'x', slice(11.0 * working_unit, 12.0 * working_unit))
    assert sc.is_equal(out.coords['x'], da['x', -1:].coords['x'])  #


def test_slice_range_on_point_coords_2D():
    data = sc.Variable(['y', 'x'], values=np.arange(100).reshape(5, 20))
    x = sc.Variable(['x'], values=np.arange(-10.0, 10.0))
    y = sc.Variable(['y'], values=np.arange(-2.5, 2.5))
    assert data.shape[0] == y.shape[0]  # Ensure working with points
    assert data.shape[1] == x.shape[0]  # Ensure working with points
    da = sc.DataArray(data=data, coords={'x': x, 'y': y})

    out = sc.slice(da, 'x', slice(-10.0 * working_unit, 10.0 * working_unit))
    # assert no-effect slicing
    assert sc.is_equal(da, out)
    # Test sc.slice x range by value
    out = sc.slice(da, 'x', slice(-10.0 * working_unit, 0.0 * working_unit))
    assert sc.is_equal(out.coords['y'],
                       da.coords['y'])  # unaffected by x-value slicing
    assert sc.is_equal(out.coords['x'], da.coords['x']['x', 0:10])
    # Test sc.slice y range by value
    out = sc.slice(da, 'y', slice(-2.5 * working_unit, 0.501 * working_unit))
    assert sc.is_equal(out.coords['x'],
                       da.coords['x'])  # unaffected by y-value slicing
    assert sc.is_equal(out.coords['y'], da.coords['y']['y', 0:4])


def test_2d_coord_unsupported():
    coord2d = sc.Variable(['y', 'x'], values=np.arange(10).reshape(5, 2))
    data = coord2d.copy()
    da = sc.DataArray(data=data, coords={'p': coord2d})
    with pytest.raises(RuntimeError):
        sc.slice(da, coord_name='p')


def test_coord_must_be_sorted_ascending():
    asc_coord = _make_1d_data_array(begin=0.0, end=4.0, dim_name='x')
    sc.slice(asc_coord, 'x')  # No throw. Sorted ascending
    desc_coord = asc_coord.copy()
    desc_coord.coords['x'].values = np.flip(desc_coord.coords['x'].values)
    assert sc.is_sorted(desc_coord.coords['x'], 'x',
                        order='descending')  # sanity check, 'x'
    with pytest.raises(RuntimeError):
        sc.slice(desc_coord, 'x')


def test_slice_point_on_point_coords_1D():
    #    Data Values           [0.0][1.0] ... [8.0][9.0]
    #    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

    da = _make_1d_data_array(begin=3.0,
                             end=13.0,
                             dim_name='x',
                             bin_edges=False)
    # test no-effect slicing
    # Test start on left boundary (closed on left), so includes boundary
    out = sc.slice(da, 'x', 3.0 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', 0].attrs['x'])
    # Test point slice between points throws
    with pytest.raises(RuntimeError):
        sc.slice(da, 'x',
                 3.5 * working_unit)  # No sensible return. Must throw.
    # Test start on right boundary
    out = sc.slice(da, 'x', 12.0 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', -1].attrs['x'])
    # Test start outside right boundary throws
    with pytest.raises(RuntimeError):
        out = sc.slice(da, 'x', 12.1 * working_unit)


def test_slice_point_on_edge_coords_1D():
    #    Data Values            [0.0] ...       [9.0]
    #    Coord Values (edges) [3.0][4.0] ... [11.0][12.0]

    da = _make_1d_data_array(begin=3.0, end=13.0, dim_name='x', bin_edges=True)
    # test no-effect slicing
    # Test start on left boundary (closed on left), so includes boundary
    out = sc.slice(da, 'x', 3.0 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', 0].attrs['x'])
    # Same as above, takes lower bounds of bin so same bin
    out = sc.slice(da, 'x', 3.5 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', 0].attrs['x'])
    # Next bin
    out = sc.slice(da, 'x', 4.0 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', 1].attrs['x'])
    # Last bin
    out = sc.slice(da, 'x', 11.9 * working_unit)
    assert sc.is_equal(out.attrs['x'], da['x', -1].attrs['x'])
    # (closed on right) so out of bounds
    with pytest.raises(RuntimeError):
        out = sc.slice(da, 'x', 12.0 * working_unit)
    # out of bounds for left for completeness
    with pytest.raises(RuntimeError):
        out = sc.slice(da, 'x', 2.99 * working_unit)
