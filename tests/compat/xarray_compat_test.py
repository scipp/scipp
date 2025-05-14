# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc
from scipp.compat import from_xarray, to_xarray

xr = pytest.importorskip('xarray')


def test_from_xarray_variable() -> None:
    xr_var = xr.Variable(
        dims=("y", "x"), data=np.arange(12.0).reshape(4, 3), attrs={"units": "m"}
    )
    sc_var = from_xarray(xr_var)

    assert isinstance(sc_var, sc.Variable)
    assert sc_var.sizes == {"x": 3, "y": 4}
    assert sc_var.unit == "m"
    assert np.array_equal(sc_var.values, xr_var.values)
    assert sc_var.variances is None


def test_from_xarray_empty_attrs_dataarray() -> None:
    xr_da = xr.DataArray(data=np.zeros((1,)), dims={"x"}, attrs={})

    sc_da = from_xarray(xr_da)

    assert isinstance(sc_da, sc.DataArray)
    assert len(sc_da.dims) == 1
    assert "x" in sc_da.dims
    assert len(sc_da.masks) == 0


def test_from_xarray_attrs_dataarray_warns_if_attrs_dropped() -> None:
    xr_da = xr.DataArray(
        data=np.zeros((1,)),
        dims=["x"],
        attrs={
            "attrib_int": 5,
            "attrib_float": 6.54321,
            "attrib_str": "test-string",
        },
    )

    with pytest.warns(UserWarning):
        _ = from_xarray(xr_da)


def test_from_xarray_converts_names_to_strings_in_dataarray() -> None:
    a = xr.Variable(dims=['y', 'x'], data=np.arange(12.0).reshape(4, 3))
    x = xr.Variable(dims=['x'], data=np.arange(3.0))
    y = xr.Variable(dims=['y'], data=np.arange(4.0))
    xr_da = xr.DataArray(a, coords={'x': x, 0: y})

    sc_da = from_xarray(xr_da)

    assert isinstance(sc_da, sc.DataArray)
    assert np.array_equal(sc_da.coords["0"].values, np.arange(4.0))


def test_from_xarray_named_dataarray() -> None:
    xr_da = xr.DataArray(data=np.zeros((1,)), dims={"x"}, name="my-test-dataarray")

    sc_da = from_xarray(xr_da)

    assert isinstance(sc_da, sc.DataArray)
    assert sc_da.name == "my-test-dataarray"


def test_from_xarray_1d_1_element_dataarray() -> None:
    xr_da = xr.DataArray(data=np.zeros((1,)), dims=["x"], attrs={})

    sc_da = from_xarray(xr_da)

    assert isinstance(sc_da, sc.DataArray)
    assert sc.identical(sc_da, sc.DataArray(data=sc.zeros(dims=["x"], shape=(1,))))


def test_from_xarray_1d_100_element_dataarray() -> None:
    xr_da = xr.DataArray(data=np.zeros((100,)), dims=["x"], attrs={})

    sc_da = from_xarray(xr_da)

    assert isinstance(sc_da, sc.DataArray)
    assert sc.identical(sc_da, sc.DataArray(data=sc.zeros(dims=["x"], shape=(100,))))


def test_from_xarray_2d_100x100_element_dataarray() -> None:
    xr_da = xr.DataArray(data=np.zeros((100, 100)), dims=["x", "y"], attrs={})

    sc_da = from_xarray(xr_da)

    assert sc.identical(
        sc_da, sc.DataArray(data=sc.zeros(dims=["x", "y"], shape=(100, 100)))
    )


def test_from_xarray_dataset_with_data() -> None:
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(data=np.zeros((50,)), dims=["x"], attrs={}),
            "array2": xr.DataArray(data=np.zeros((50,)), dims=["x"], attrs={}),
        }
    )

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.DataArray(data=sc.zeros(dims=["x"], shape=(50,))),
            "array2": sc.DataArray(data=sc.zeros(dims=["x"], shape=(50,))),
        }
    )

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_units() -> None:
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(
                data=np.zeros((50,)), dims=["x"], attrs={"units": "m"}
            ),
            "array2": xr.DataArray(
                data=np.zeros((50,)), dims=["x"], attrs={"units": "s"}
            ),
        }
    )

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.DataArray(
                data=sc.zeros(dims=["x"], shape=(50,), unit=sc.Unit("m"))
            ),
            "array2": sc.DataArray(
                data=sc.zeros(dims=["x"], shape=(50,), unit=sc.Unit("s"))
            ),
        }
    )

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_non_indexed_coords() -> None:
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(
                data=np.zeros((50,)),
                dims=["x"],
                coords={
                    "x": np.arange(50, dtype="int64"),
                },
            ),
            "array2": xr.DataArray(
                data=np.zeros((50,)),
                dims=["x"],
                coords={
                    "x": np.arange(50, dtype="int64"),
                    "y": (
                        "x",
                        np.arange(0, 100, 2, dtype="int64"),
                    ),  # y is a non-index coord
                },
            ),
        }
    )

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.zeros(dims=["x"], shape=(50,), dtype="float64"),
            "array2": sc.zeros(dims=["x"], shape=(50,), dtype="float64"),
        },
        coords={
            "x": sc.arange("x", 50, dtype="int64"),
            "y": sc.arange("x", 0, 100, 2, dtype="int64"),
        },
    )
    reference_ds.coords.set_aligned('y', False)

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_extra_coord() -> None:
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(
                data=np.zeros((50,)),
                dims=["x"],
                coords={
                    "x": np.arange(50, dtype="int64"),
                },
            ),
            "array2": xr.DataArray(
                data=np.zeros((50,)),
                dims=["x"],
                coords={"x": np.arange(50, dtype="int64")},
            ),
        }
    )
    xr_ds.coords["z"] = xr.Variable(dims="x", data=np.arange(50.0))

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.zeros(dims=["x"], shape=(50,), dtype="float64"),
            "array2": sc.zeros(dims=["x"], shape=(50,), dtype="float64"),
        },
        coords={
            "x": sc.arange("x", 50, dtype="int64"),
            "z": sc.arange("x", 50.0),
        },
    )
    reference_ds.coords.set_aligned("z", False)

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_attrs_warns() -> None:
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(
                data=np.zeros((50,)),
                dims=["x"],
                coords={
                    "x": np.arange(50, dtype="int64"),
                },
            )
        },
        attrs={'a': 1, 'b': 2},
    )
    with pytest.warns(UserWarning):
        sc_ds = from_xarray(xr_ds)

    assert isinstance(sc_ds, sc.Dataset)
    assert len(sc_ds) == 1
    assert len(sc_ds.coords) == 1


@pytest.mark.filterwarnings("ignore:.*attributes.*:UserWarning")
def test_from_xarray_dataset_with_only_attrs() -> None:
    xr_ds = xr.Dataset(attrs={'a': 1, 'b': 2})
    sc_ds = from_xarray(xr_ds)

    assert isinstance(sc_ds, sc.Dataset)
    assert len(sc_ds) == 0
    assert sc_ds.sizes == {}


def test_to_xarray_variable() -> None:
    sc_var = sc.arange('aux', 0.0, 90, 2, unit='m').fold(
        'aux', sizes={'xx': 5, 'yy': 9}
    )
    xr_var = to_xarray(sc_var)
    assert xr_var.sizes == {"yy": 9, "xx": 5}
    assert xr_var.attrs["units"] == "m"
    assert np.array_equal(xr_var.values, sc_var.values)


def test_to_xarray_variable_variances_dropped() -> None:
    sc_var = sc.arange('aux', 0.0, 90, 2, unit='m').fold(
        'aux', sizes={'xx': 5, 'yy': 9}
    )
    sc_var.variances = sc_var.values * 0.1
    with pytest.warns(UserWarning):
        xr_var = to_xarray(sc_var)
    assert xr_var.sizes == {"yy": 9, "xx": 5}
    assert xr_var.attrs["units"] == "m"
    assert np.array_equal(xr_var.values, sc_var.values)


def test_to_xarray_dataarray() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'xx': 5, 'yy': 9}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='s'),
            'yy': sc.arange('yy', 9.0, unit='µK'),
        },
    )
    xr_da = to_xarray(sc_da)
    assert xr_da.dims == sc_da.dims
    assert xr_da.shape == sc_da.shape
    assert all(x in xr_da.coords for x in ["xx", "yy"])
    assert xr_da.attrs['units'] == 'counts'
    assert np.array_equal(xr_da.values, sc_da.values)
    assert np.array_equal(xr_da.coords['xx'].values, sc_da.coords['xx'].values)
    assert np.array_equal(xr_da.coords['yy'].values, sc_da.coords['yy'].values)


def test_to_xarray_dataarray_2d_coord() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts'),
        coords={'a2dcoord': sc.arange('aux', 45.0, 90.0)},
    ).fold('aux', sizes={'xx': 5, 'yy': 9})
    xr_da = to_xarray(sc_da)
    assert xr_da.coords['a2dcoord'].dims == ("xx", "yy")
    assert np.array_equal(
        xr_da.coords['a2dcoord'].values, sc_da.coords['a2dcoord'].values
    )


@pytest.mark.parametrize('coord', ['xx', 'yy'])
def test_to_xarray_dataarray_with_unaligned_coords(coord: str) -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'yy': 9, 'xx': 5}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='m'),
            'yy': sc.arange('yy', 9.0, unit='m'),
        },
    )
    sc_da.coords.set_aligned(coord, False)
    xr_da = to_xarray(sc_da)
    assert coord in xr_da.coords
    assert coord not in xr_da.indexes


def test_to_xarray_dataarray_with_unaligned_coords_multiple() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'yy': 9, 'xx': 5}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='m'),
            'yy': sc.arange('yy', 9.0, unit='m'),
        },
    )
    sc_da.coords.set_aligned('xx', False)
    sc_da.coords.set_aligned('yy', False)
    xr_da = to_xarray(sc_da)
    assert 'xx' in xr_da.coords
    assert 'xx' not in xr_da.indexes
    assert 'yy' in xr_da.coords
    assert 'yy' not in xr_da.indexes


def test_to_xarray_dataarray_fails_on_bin_edges() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'xx': 5, 'yy': 9}
        ),
        coords={'xx': sc.arange('xx', 6.0, unit='s')},
    )
    with pytest.raises(ValueError, match='bin edges'):
        _ = to_xarray(sc_da)


def test_to_xarray_dataarray_fails_on_binned_data() -> None:
    buffer = sc.DataArray(
        sc.ones(sizes={'event': 5}), coords={'id': sc.arange('event', 5)}
    )
    binned = buffer.bin(id=3)
    with pytest.raises(ValueError, match='binned data'):
        _ = to_xarray(binned)


def test_to_xarray_dataarray_masks_dropped() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'xx': 5, 'yy': 9}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='s'),
            'yy': sc.arange('yy', 9.0, unit='µK'),
        },
        masks={'m': sc.array(dims=['xx'], values=[False, False, True, False, True])},
    )
    with pytest.warns(UserWarning):
        xr_da = to_xarray(sc_da)
    assert xr_da.dims == sc_da.dims
    assert xr_da.shape == sc_da.shape
    assert all(x in xr_da.coords for x in ["xx", "yy"])
    assert xr_da.attrs['units'] == 'counts'
    assert np.array_equal(xr_da.values, sc_da.values)


def test_dataarray_round_trip() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'xx': 5, 'yy': 9}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='s'),
            'yy': sc.arange('yy', 9.0, unit='µK'),
        },
    )
    assert sc.identical(sc_da, from_xarray(to_xarray(sc_da)))


def test_to_xarray_dataset() -> None:
    sc_da = sc.DataArray(
        sc.arange('aux', 0.0, 90, 2, unit='counts').fold(
            'aux', sizes={'xx': 5, 'yy': 9}
        ),
        coords={
            'xx': sc.arange('xx', 5.0, unit='s'),
            'yy': sc.arange('yy', 9.0, unit='µK'),
        },
    )
    sc_ds = sc.Dataset({'a': sc_da, 'b': 2.0 * sc_da})
    xr_ds = to_xarray(sc_ds)
    assert all(x in xr_ds.coords for x in ["xx", "yy"])
    assert all(x in xr_ds for x in ["a", "b"])
    assert all(xr_ds[x].dims == sc_ds[x].dims for x in ["a", "b"])
    assert all(xr_ds[x].shape == sc_ds[x].shape for x in ["a", "b"])


def test_repr_html_includes_module_name_dataarray() -> None:
    html = sc.DataGroup(
        a=xr.DataArray(dims=['z'], data=[1.0, 2.0, 3.0, 4.0])
    )._repr_html_()
    assert 'xarray</span></div>' in html


def test_repr_html_has_size_dataarray() -> None:
    html = sc.DataGroup(
        a=xr.DataArray(dims=['z'], data=[1.0, 2.0, 3.0, 4.0])
    )._repr_html_()
    assert '<div class=\'dg-detail-shape\'>(z: 4)</div>' in html


def test_repr_html_includes_module_name_dataset() -> None:
    html = sc.DataGroup(
        a=xr.Dataset({'a': xr.DataArray(dims=['z'], data=[1.0, 2.0, 3.0, 4.0])})
    )._repr_html_()
    assert 'xarray</span></div>' in html


def test_repr_html_has_size_dataset() -> None:
    html = sc.DataGroup(
        a=xr.Dataset({'a': xr.DataArray(dims=['z'], data=[1.0, 2.0, 3.0, 4.0])})
    )._repr_html_()
    assert '<div class=\'dg-detail-shape\'>(z: 4)</div>' in html
