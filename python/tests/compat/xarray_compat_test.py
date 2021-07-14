import pytest

import numpy
import xarray
import scipp
from scipp.compat.xarray_compat import from_xarray


def test_empty_attrs_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray(x_dataarray)

    assert len(sc_dataarray.attrs) == 0

    assert len(sc_dataarray.dims) == 1
    assert "x" in sc_dataarray.dims

    assert len(sc_dataarray.masks) == 0


def test_attrs_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   attrs={
                                       "attrib_int": 5,
                                       "attrib_float": 6.54321,
                                       "attrib_str": "test-string",
                                   })

    sc_dataarray = from_xarray(x_dataarray)

    assert sc_dataarray.attrs["attrib_int"].values == 5
    assert sc_dataarray.attrs["attrib_float"].values == 6.54321
    assert sc_dataarray.attrs["attrib_str"].values == "test-string"


def test_named_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   name="my-test-dataarray")

    sc_dataarray = from_xarray(x_dataarray)

    assert sc_dataarray.name == "my-test-dataarray"


def test_1d_1_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x"],
                                               shape=(1, )).values).all()


def test_1d_100_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((100, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x"],
                                               shape=(100, )).values).all()


def test_2d_100x100_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((100, 100)),
                                   dims={"x", "y"},
                                   attrs={})

    sc_dataarray = from_xarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x", "y"],
                                               shape=(100, 100)).values).all()


def test_empty_dataset():
    x_dataset = xarray.Dataset(data_vars={})

    sc_dataset = from_xarray(x_dataset)

    assert len(sc_dataset) == 0


def test_dataset_with_attrs():
    x_dataset = xarray.Dataset(data_vars={},
                               attrs={
                                   "int": 5,
                                   "float": 6.54321,
                                   "str": "test-string",
                               })

    sc_dataset = from_xarray(x_dataset)

    assert sc_dataset["attrib_int"].values == 5
    assert sc_dataset["attrib_float"].values == 6.54321
    assert sc_dataset["attrib_str"].values == "test-string"


def test_dataset_with_colliding_data_and_attrs_throws_error():
    x_dataset = xarray.Dataset(
        data_vars={"attrib_test1": xarray.Variable(["x"], numpy.zeros((1, )))},
        attrs={"test1": 5})

    with pytest.raises(ValueError):
        from_xarray(x_dataset)


def test_dataset_with_data():
    x_dataset = xarray.Dataset(
        data_vars={
            "array1":
            xarray.DataArray(data=numpy.zeros((100, )), dims={"x"}, attrs={}),
            "array2":
            xarray.DataArray(data=numpy.zeros((50, )), dims={"y"}, attrs={}),
        })

    sc_dataset = from_xarray(x_dataset)

    assert (sc_dataset["array1"].values == scipp.zeros(
        dims=["x"], shape=(100, )).values).all()
    assert (sc_dataset["array2"].values == scipp.zeros(
        dims=["y"], shape=(50, )).values).all()
