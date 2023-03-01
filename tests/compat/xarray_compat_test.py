# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import numpy as np
import pytest
import xarray as xr

import scipp as sc
from scipp.compat import from_xarray, to_xarray

from ..factory import make_binned_data_array, make_dense_data_array, \
    make_dense_dataset, make_variable  # NOQA


def test_from_xarray_variable():
    xr_var = xr.Variable(dims=("y", "x"),
                         data=np.arange(12.).reshape(4, 3),
                         attrs={"units": "m"})
    sc_var = from_xarray(xr_var)

    assert sc_var.sizes == {"x": 3, "y": 4}
    assert sc_var.unit == "m"
    assert np.array_equal(sc_var.values, xr_var.values)
    assert sc_var.variances is None


def test_from_xarray_empty_attrs_dataarray():
    xr_da = xr.DataArray(data=np.zeros((1, )), dims={"x"}, attrs={})

    sc_da = from_xarray(xr_da)

    assert len(sc_da.attrs) == 0

    assert len(sc_da.dims) == 1
    assert "x" in sc_da.dims

    assert len(sc_da.masks) == 0


def test_from_xarray_attrs_dataarray():
    xr_da = xr.DataArray(data=np.zeros((1, )),
                         dims=["x"],
                         attrs={
                             "attrib_int": 5,
                             "attrib_float": 6.54321,
                             "attrib_str": "test-string",
                         })

    sc_da = from_xarray(xr_da)

    assert sc_da.attrs["attrib_int"].values == 5
    assert sc_da.attrs["attrib_float"].values == 6.54321
    assert sc_da.attrs["attrib_str"].values == "test-string"


def test_from_xarray_raises_when_attrs_shadowing():
    xr_da = xr.DataArray(data=np.zeros(10), dims=["x"], coords={"x": np.arange(10)})
    xr_da.coords['aux'] = xr.Variable(dims=['x'], data=np.arange(10.))
    xr_da.attrs['aux'] = 'this is an attribute'
    with pytest.raises(ValueError):
        _ = from_xarray(xr_da)


def test_from_xarray_converts_names_to_strings_in_dataarray():
    a = xr.Variable(dims=['y', 'x'], data=np.arange(12.).reshape(4, 3))
    x = xr.Variable(dims=['x'], data=np.arange(3.))
    y = xr.Variable(dims=['y'], data=np.arange(4.))
    xr_da = xr.DataArray(a,
                         coords={'x': x, 0: y},
                         attrs={
                             1: 6.54321,
                             2: "test-string",
                         })

    sc_da = from_xarray(xr_da)

    assert np.array_equal(sc_da.attrs["0"].values, np.arange(4.))
    assert sc_da.attrs["1"].values == 6.54321
    assert sc_da.attrs["2"].values == "test-string"


def test_from_xarray_named_dataarray():
    xr_da = xr.DataArray(data=np.zeros((1, )), dims={"x"}, name="my-test-dataarray")

    sc_da = from_xarray(xr_da)

    assert sc_da.name == "my-test-dataarray"


def test_from_xarray_1d_1_element_dataarray():
    xr_da = xr.DataArray(data=np.zeros((1, )), dims=["x"], attrs={})

    sc_da = from_xarray(xr_da)

    assert sc.identical(sc_da, sc.DataArray(data=sc.zeros(dims=["x"], shape=(1, ))))


def test_from_xarray_1d_100_element_dataarray():
    xr_da = xr.DataArray(data=np.zeros((100, )), dims=["x"], attrs={})

    sc_da = from_xarray(xr_da)

    assert sc.identical(sc_da, sc.DataArray(data=sc.zeros(dims=["x"], shape=(100, ))))


def test_from_xarray_2d_100x100_element_dataarray():
    xr_da = xr.DataArray(data=np.zeros((100, 100)), dims=["x", "y"], attrs={})

    sc_da = from_xarray(xr_da)

    assert sc.identical(sc_da,
                        sc.DataArray(data=sc.zeros(dims=["x", "y"], shape=(100, 100))))


def test_from_xarray_empty_dataset():
    xr_ds = xr.Dataset(data_vars={})

    sc_ds = from_xarray(xr_ds)

    assert sc.identical(sc_ds, sc.Dataset(data={}))


def test_from_xarray_dataset_with_data():
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(data=np.zeros((100, )), dims=["x"], attrs={}),
            "array2": xr.DataArray(data=np.zeros((50, )), dims=["y"], attrs={}),
        })

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.DataArray(data=sc.zeros(dims=["x"], shape=(100, ))),
            "array2": sc.DataArray(data=sc.zeros(dims=["y"], shape=(50, ))),
        })

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_units():
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(
                data=np.zeros((100, )), dims=["x"], attrs={"units": "m"}),
            "array2": xr.DataArray(
                data=np.zeros((50, )), dims=["y"], attrs={"units": "s"}),
        })

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(
        data={
            "array1": sc.DataArray(
                data=sc.zeros(dims=["x"], shape=(100, ), unit=sc.Unit("m"))),
            "array2": sc.DataArray(
                data=sc.zeros(dims=["y"], shape=(50, ), unit=sc.Unit("s"))),
        })

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_non_indexed_coords():
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(data=np.zeros((100, )),
                                   dims=["x"],
                                   coords={
                                       "x": np.arange(100, dtype="int64"),
                                   }),
            "array2": xr.DataArray(
                data=np.zeros((50, )),
                dims=["y"],
                coords={
                    "y": np.arange(50, dtype="int64"),
                    "z": (
                        "y",
                        np.arange(0, 100, 2, dtype="int64")),  # z is a non-index coord
                }),
        })

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(data={
        "array1": sc.DataArray(
            data=sc.zeros(dims=["x"], shape=(100, ), dtype="float64")),
        "array2": sc.DataArray(data=sc.zeros(dims=["y"], shape=(50, ), dtype="float64"),
                               attrs={"z": sc.arange("y", 0, 100, 2, dtype="int64")}),
    },
                              coords={
                                  "x": sc.arange("x", 100, dtype="int64"),
                                  "y": sc.arange("y", 50, dtype="int64"),
                              })

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_extra_coord():
    xr_ds = xr.Dataset(
        data_vars={
            "array1": xr.DataArray(data=np.zeros((100, )),
                                   dims=["x"],
                                   coords={
                                       "x": np.arange(100, dtype="int64"),
                                   }),
            "array2": xr.DataArray(data=np.zeros((50, )),
                                   dims=["y"],
                                   coords={"y": np.arange(50, dtype="int64")}),
        })
    xr_ds.coords["z"] = xr.Variable(dims="z", data=np.arange(66.))

    sc_ds = from_xarray(xr_ds)

    reference_ds = sc.Dataset(data={
        "array1": sc.DataArray(
            data=sc.zeros(dims=["x"], shape=(100, ), dtype="float64")),
        "array2": sc.DataArray(
            data=sc.zeros(dims=["y"], shape=(50, ), dtype="float64")),
    },
                              coords={
                                  "x": sc.arange("x", 100, dtype="int64"),
                                  "y": sc.arange("y", 50, dtype="int64"),
                                  "z": sc.arange("z", 66.)
                              })

    assert sc.identical(sc_ds, reference_ds)


def test_from_xarray_dataset_with_attrs_warns():
    xr_ds = xr.Dataset(attrs={'a': 1, 'b': 2})
    with pytest.warns(UserWarning):
        sc_ds = from_xarray(xr_ds)
    assert len(sc_ds) == 0
    assert len(sc_ds.coords) == 0


def test_to_xarray_variable():
    sc_var = make_variable(ndim=2, unit='m')
    xr_var = to_xarray(sc_var)
    assert xr_var.sizes == {"yy": 50, "xx": 40}
    assert xr_var.attrs["units"] == "m"
    assert np.array_equal(xr_var.values, sc_var.values)


def test_to_xarray_variable_variances_dropped():
    sc_var = make_variable(ndim=2, unit='m', with_variance=True)
    with pytest.warns(UserWarning):
        xr_var = to_xarray(sc_var)
    assert xr_var.sizes == {"yy": 50, "xx": 40}
    assert xr_var.attrs["units"] == "m"
    assert np.array_equal(xr_var.values, sc_var.values)


def test_to_xarray_dataarray():
    sc_da = make_dense_data_array(ndim=2)
    xr_da = to_xarray(sc_da)
    assert xr_da.dims == sc_da.dims
    assert xr_da.shape == sc_da.shape
    assert all(x in xr_da.coords for x in ["xx", "yy"])
    assert xr_da.attrs['units'] == 'counts'
    assert np.array_equal(xr_da.values, sc_da.values)


def test_to_xarray_dataarray_2d_coord():
    sc_da = make_dense_data_array(ndim=2)
    sc_da.coords['a2dcoord'] = sc.fold(sc.arange('_', float(np.prod(sc_da.shape))),
                                       dim='_',
                                       sizes=sc_da.sizes)
    xr_da = to_xarray(sc_da)
    assert xr_da.coords['a2dcoord'].dims == ("yy", "xx")


def test_to_xarray_dataarray_with_attrs():
    sc_da = make_dense_data_array(ndim=2, attrs=True)
    xr_da = to_xarray(sc_da)
    assert "attr" in xr_da.coords
    assert "attr" not in xr_da.indexes


def test_to_xarray_dataarray_fails_on_bin_edges():
    sc_da = make_dense_data_array(ndim=2, binedges=True)
    with pytest.raises(ValueError):
        _ = to_xarray(sc_da)


def test_to_xarray_dataarray_fails_on_binned_data():
    sc_da = make_binned_data_array(ndim=2)
    with pytest.raises(ValueError):
        _ = to_xarray(sc_da)


def test_to_xarray_dataarray_masks_dropped():
    sc_da = make_dense_data_array(ndim=2, masks=True)
    with pytest.warns(UserWarning):
        xr_da = to_xarray(sc_da)
    assert xr_da.dims == sc_da.dims
    assert xr_da.shape == sc_da.shape
    assert all(x in xr_da.coords for x in ["xx", "yy"])
    assert xr_da.attrs['units'] == 'counts'
    assert np.array_equal(xr_da.values, sc_da.values)


def test_dataarray_round_trip():
    sc_da = make_dense_data_array(ndim=2)
    assert sc.identical(sc_da, from_xarray(to_xarray(sc_da)))


def test_to_xarray_dataset():
    sc_ds = make_dense_dataset(ndim=2)
    xr_ds = to_xarray(sc_ds)
    assert all(x in xr_ds.coords for x in ["xx", "yy"])
    assert all(x in xr_ds for x in ["a", "b"])
    assert all(xr_ds[x].dims == sc_ds[x].dims for x in ["a", "b"])
    assert all(xr_ds[x].shape == sc_ds[x].shape for x in ["a", "b"])


def test_to_xarray_dataset_with_extra_coord():
    a = sc.Dataset(coords={"extra_coord": sc.arange("extra_dim", 10.)})
    b = make_dense_dataset(ndim=2)
    sc_ds = sc.merge(a, b)
    xr_ds = to_xarray(sc_ds)
    assert "extra_dim" in xr_ds.dims
    assert "extra_coord" in xr_ds.coords
    assert xr_ds.sizes["extra_dim"] == 10
