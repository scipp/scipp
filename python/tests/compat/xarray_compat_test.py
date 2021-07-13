import numpy
import xarray
import scipp
from scipp.compat.xarray_compat import from_xarray_dataarray


def test_empty_attrs_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray_dataarray(x_dataarray)

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

    sc_dataarray = from_xarray_dataarray(x_dataarray)

    assert sc_dataarray.attrs["attrib_int"].values == 5
    assert sc_dataarray.attrs["attrib_float"].values == 6.54321
    assert sc_dataarray.attrs["attrib_str"].values == "test-string"


def test_named_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   name="my-test-dataarray")

    sc_dataarray = from_xarray_dataarray(x_dataarray)

    assert sc_dataarray.name == "my-test-dataarray"


def test_1d_1_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((1, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray_dataarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x"],
                                               shape=(1, )).values).all()


def test_1d_100_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((100, )),
                                   dims={"x"},
                                   attrs={})

    sc_dataarray = from_xarray_dataarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x"],
                                               shape=(100, )).values).all()


def test_2d_100x100_element_dataarray():
    x_dataarray = xarray.DataArray(data=numpy.zeros((100, 100)),
                                   dims={"x", "y"},
                                   attrs={})

    sc_dataarray = from_xarray_dataarray(x_dataarray)

    assert (sc_dataarray.values == scipp.zeros(dims=["x", "y"],
                                               shape=(100, 100)).values).all()
