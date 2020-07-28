# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def test_variable_to_dict():
    var = sc.Variable(dims=["x"],
                      values=np.arange(10.),
                      variances=np.random.random(10),
                      unit=sc.units.m)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == var.dims
    assert var_dict["shape"] == var.shape
    assert np.array_equal(var_dict["values"], var.values)
    assert np.array_equal(var_dict["variances"], var.variances)
    assert var_dict["unit"] == var.unit
    assert var_dict["dtype"] == var.dtype


def test_variable_0D_to_dict():
    var = 12.0 * sc.units.one
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == []
    assert var_dict["shape"] == []
    assert var_dict["value"] == 12.0


def test_variable_vector_to_dict():
    var = sc.Variable(['x'],
                      values=np.random.random([10, 3]),
                      dtype=sc.dtype.vector_3_float64)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ['x']
    assert var_dict["shape"] == [10]
    assert var_dict["values"].shape == (10, 3)
    assert var_dict["dtype"] == sc.dtype.vector_3_float64


def test_variable_0D_vector_to_dict():
    var = sc.Variable(value=[1, 2, 3], dtype=sc.dtype.vector_3_float64)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == []
    assert var_dict["shape"] == []
    assert np.array_equal(var_dict["value"], [1, 2, 3])
    assert var_dict["dtype"] == sc.dtype.vector_3_float64


def test_variable_matrix_to_dict():
    data = np.array([
        np.arange(9.0).reshape(3, 3),
        np.arange(5.0, 14.0).reshape(3, 3),
        np.arange(1.0, 10.0).reshape(3, 3),
        np.arange(2.0, 11.0).reshape(3, 3)
    ])
    var = sc.Variable(['x'],
                      values=data,
                      unit=sc.units.m,
                      dtype=sc.dtype.matrix_3_float64)
    var_dict = sc.to_dict(var)
    assert var_dict["shape"] == [4]
    assert var_dict["values"].shape == (4, 3, 3)
    assert var_dict["dtype"] == sc.dtype.matrix_3_float64


def test_variable_0D_matrix_to_dict():
    var = sc.Variable(value=np.arange(1, 10).reshape(3, 3),
                      dtype=sc.dtype.matrix_3_float64)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == []
    assert var_dict["shape"] == []
    assert np.array_equal(var_dict["value"], [[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert var_dict["dtype"] == sc.dtype.matrix_3_float64


def test_variable_event_to_dict():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    var['x', 1].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 3].values = np.ones(6)
    var_dict = sc.to_dict(var)
    assert var_dict["shape"] == [4]
    assert np.array_equal(var_dict["values"][0], [1, 1, 1])
    assert np.array_equal(var_dict["values"][1], [42])
    assert len(var_dict["values"][2]) == 0
    assert np.array_equal(var_dict["values"][3], [1, 1, 1, 1, 1, 1])


def test_variable_from_dict():
    var_dict = {
        "dims": ['x'],
        "values": np.random.random(10),
        "variances": np.random.random(10)
    }
    var = sc.from_dict(var_dict)
    assert var.dims == var_dict["dims"]
    assert var.shape == [10]
    assert np.array_equal(var.values, var_dict["values"])
    assert np.array_equal(var.variances, var_dict["variances"])
    assert var.unit == sc.units.one


def test_variable_0D_from_dict():
    var_dict = {"value": 17., "variance": 0.2}
    var = sc.from_dict(var_dict)
    assert var.dims == []
    assert var.shape == []
    assert var.value == 17.0
    assert var.variance == 0.2
    assert var.unit == sc.units.one


def test_variable_vector_from_dict():
    var_dict = {
        "dims": ['x'],
        "values": np.arange(6).reshape(2, 3),
        "dtype": "vector_3_float64"
    }
    var = sc.from_dict(var_dict)
    assert var.dims == var_dict["dims"]
    assert var.shape == [2]
    assert np.array_equal(np.array(var.values), [[0, 1, 2], [3, 4, 5]])
    assert var.unit == sc.units.one
    assert var.dtype == sc.dtype.vector_3_float64


def test_variable_0D_vector_from_dict():
    var_dict = {"value": [1, 2, 3], "dtype": "vector_3_float64"}
    var = sc.from_dict(var_dict)
    assert var.dims == []
    assert var.shape == []
    assert np.array_equal(np.array(var.values), [1, 2, 3])
    assert var.unit == sc.units.one
    assert var.dtype == sc.dtype.vector_3_float64


def test_variable_matrix_from_dict():
    var_dict = {
        "dims": ['x'],
        "values": np.arange(18).reshape(2, 3, 3),
        "dtype": "matrix_3_float64"
    }
    var = sc.from_dict(var_dict)
    assert var.dims == var_dict["dims"]
    assert var.shape == [2]
    assert np.array_equal(np.array(var.values), var_dict["values"])
    assert var.unit == sc.units.one
    assert var.dtype == sc.dtype.matrix_3_float64


def test_variable_0D_matrix_from_dict():
    var_dict = {
        "value": np.arange(9).reshape(3, 3),
        "dtype": "matrix_3_float64"
    }
    var = sc.from_dict(var_dict)
    assert var.dims == []
    assert var.shape == []
    assert np.array_equal(np.array(var.value), var_dict["value"])
    assert var.unit == sc.units.one
    assert var.dtype == sc.dtype.matrix_3_float64


def test_variable_event_from_dict():
    var_dict = {
        'dims': ['x'],
        'dtype':
        'event_list_float64',
        'values':
        np.array([
            np.array([1., 1., 1.]),
            np.array([42.]),
            np.array([]),
            np.array([1., 1., 1., 1., 1., 1.])
        ])
    }
    var = sc.from_dict(var_dict)
    assert var.dims == ['x']
    assert var.shape == [4]
    assert np.array_equal(var['x', 0].values, [1., 1., 1.])
    assert np.array_equal(var['x', 1].values, [42.])
    assert len(var['x', 2].values) == 0
    assert np.array_equal(var['x', 3].values, [1., 1., 1., 1., 1., 1.])
    assert var.unit == sc.units.one
    assert var.dtype == sc.dtype.event_list_float64


def test_variable_round_trip():
    var = sc.Variable(dims=["x"],
                      values=np.arange(10.),
                      variances=np.random.random(10),
                      unit=sc.units.m)
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_0D_round_trip():
    var = 12.0 * sc.units.one
    print(sc.to_dict(var))
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_vector_round_trip():
    var = sc.Variable(['x'],
                      values=np.random.random([10, 3]),
                      dtype=sc.dtype.vector_3_float64)
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_0D_vector_round_trip():
    var = sc.Variable(value=[1, 2, 3], dtype=sc.dtype.vector_3_float64)
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_matrix_round_trip():
    data = np.array([
        np.arange(9.0).reshape(3, 3),
        np.arange(5.0, 14.0).reshape(3, 3),
        np.arange(1.0, 10.0).reshape(3, 3),
        np.arange(2.0, 11.0).reshape(3, 3)
    ])
    var = sc.Variable(['x'],
                      values=data,
                      unit=sc.units.m,
                      dtype=sc.dtype.matrix_3_float64)
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_0D_matrix_round_trip():
    var = sc.Variable(value=np.arange(1, 10).reshape(3, 3),
                      dtype=sc.dtype.matrix_3_float64)
    assert var == sc.from_dict(sc.to_dict(var))


def test_variable_event_round_trip():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    var['x', 1].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 3].values = np.ones(6)
    assert var == sc.from_dict(sc.to_dict(var))


def test_data_array_to_dict():
    da = sc.DataArray(
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10.)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
        masks={
            "amask":
            sc.Variable(dims=["y"], values=[True, True, False, True, False])
        },
        attrs={"attr1": sc.Variable(dims=["x"], values=np.random.random(10))},
        data=sc.Variable(dims=["y", "x"], values=np.random.random([5, 10])))
    da_dict = sc.to_dict(da)
    assert da_dict["data"]["dims"] == da.dims
    assert da_dict["data"]["shape"] == da.shape
    assert np.array_equal(da_dict["data"]["values"], da.values)
    assert np.array_equal(da_dict["data"]["variances"], da.variances)
    assert da_dict["data"]["unit"] == da.unit
    assert da_dict["data"]["dtype"] == da.dtype
    assert sc.from_dict(da_dict["coords"]["x"]) == da.coords["x"]
    assert sc.from_dict(da_dict["coords"]["y"]) == da.coords["y"]
    assert sc.from_dict(da_dict["masks"]["amask"]) == da.masks["amask"]
    assert sc.from_dict(da_dict["attrs"]["attr1"]) == da.attrs["attr1"]


def test_data_array_unaligned_to_dict():
    N = 50
    values = 10 * np.random.rand(N)
    da = sc.DataArray(data=sc.Variable(dims=['position'],
                                       unit=sc.units.counts,
                                       values=values,
                                       variances=values),
                      coords={
                          'position':
                          sc.Variable(
                              dims=['position'],
                              values=['site-{}'.format(i) for i in range(N)]),
                          'x':
                          sc.Variable(dims=['position'],
                                      unit=sc.units.m,
                                      values=np.random.rand(N)),
                          'y':
                          sc.Variable(dims=['position'],
                                      unit=sc.units.m,
                                      values=np.random.rand(N))
                      })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    ybins = sc.Variable(dims=['y'],
                        unit=sc.units.m,
                        values=[0.1, 0.3, 0.5, 0.7, 0.9])
    realigned = sc.realign(da, {'y': ybins, 'x': xbins})

    da_dict = sc.to_dict(realigned)
    assert "unaligned" in da_dict
    assert "data" not in da_dict
    assert sc.from_dict(da_dict["coords"]["x"]) == xbins
    assert sc.from_dict(da_dict["coords"]["y"]) == ybins
    assert sc.from_dict(
        da_dict["unaligned"]["coords"]["x"]) == realigned.unaligned.coords["x"]
    assert sc.from_dict(
        da_dict["unaligned"]["coords"]["y"]) == realigned.unaligned.coords["y"]


def test_data_array_from_dict():
    da_dict = {
        "coords": {
            "x": {
                "dims": ["x"],
                "values": np.arange(10)
            },
            "y": {
                "dims": ["y"],
                "values": np.arange(5),
                "unit": sc.units.m
            },
        },
        "masks": {
            "amask": {
                "dims": ["y"],
                "values": [True, True, False, True, False]
            }
        },
        "attrs": {
            "attr1": {
                "dims": ["x"],
                "values": np.random.random(10)
            }
        },
        "data": {
            "dims": ["y", "x"],
            "values": np.random.random([5, 10])
        }
    }
    da = sc.from_dict(da_dict)
    assert da.coords["x"] == sc.from_dict(da_dict["coords"]["x"])
    assert da.coords["y"] == sc.from_dict(da_dict["coords"]["y"])
    assert da.masks["amask"] == sc.from_dict(da_dict["masks"]["amask"])
    assert da.attrs["attr1"] == sc.from_dict(da_dict["attrs"]["attr1"])
    assert da.data == sc.from_dict(da_dict["data"])


def test_data_array_round_trip():
    da = sc.DataArray(coords={
        "x":
        sc.Variable(dims=["x"], values=np.arange(10.)),
        "y":
        sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
    },
                      masks={
                          "amask":
                          sc.Variable(dims=["y"],
                                      values=[True, True, False, True, False])
                      },
                      data=sc.Variable(dims=["y", "x"],
                                       values=np.random.random([5, 10])))
    assert da == sc.from_dict(sc.to_dict(da))


def test_dataset_to_dict():
    ds = sc.Dataset()
    ds.coords["x"] = sc.Variable(dims=["x"], values=np.arange(10))
    ds.coords["y"] = sc.Variable(dims=["y"],
                                 values=np.arange(5),
                                 unit=sc.units.m)
    ds["a"] = sc.Variable(dims=["y", "x"], values=np.random.random([5, 10]))
    ds["b"] = sc.Variable(dims=["y", "x"],
                          values=np.random.random([5, 10]),
                          variances=np.random.random([5, 10]),
                          unit=sc.units.s)
    ds.masks["amask"] = sc.Variable(dims=["y"],
                                    values=[True, True, False, True, False])
    # Note that attributes complicate things here, as they are duplicated in
    # each entry during the conversion to dict. So for now, we leave attributes
    # out.
    ds_dict = sc.to_dict(ds)
    assert sc.from_dict(ds_dict["a"]) == ds["a"]
    assert sc.from_dict(ds_dict["b"]) == ds["b"]


def test_dataset_from_dict():
    ds_dict = {
        "a": {
            "coords": {
                "x": {
                    "dims": ["x"],
                    "values": np.arange(10)
                },
                "y": {
                    "dims": ["y"],
                    "values": np.arange(5),
                    "unit": sc.units.m
                },
            },
            "masks": {
                "amask": {
                    "dims": ["y"],
                    "values": [True, True, False, True, False]
                }
            },
            "data": {
                "dims": ["y", "x"],
                "values": np.random.random([5, 10])
            }
        },
        "b": {
            "coords": {
                "x": {
                    "dims": ["x"],
                    "values": np.arange(10)
                },
                "y": {
                    "dims": ["y"],
                    "values": np.arange(5),
                    "unit": sc.units.m
                },
            },
            "masks": {
                "amask": {
                    "dims": ["y"],
                    "values": [True, True, False, True, False]
                }
            },
            "data": {
                "dims": ["y", "x"],
                "values": np.random.random([5, 10]),
                "variances": np.random.random([5, 10])
            }
        }
    }
    ds = sc.from_dict(ds_dict)
    assert ds["a"] == sc.from_dict(ds_dict["a"])
    assert ds["b"] == sc.from_dict(ds_dict["b"])


def test_dataset_round_trip():
    ds = sc.Dataset()
    ds.coords["x"] = sc.Variable(dims=["x"], values=np.arange(10))
    ds.coords["y"] = sc.Variable(dims=["y"],
                                 values=np.arange(5),
                                 unit=sc.units.m)
    ds["a"] = sc.Variable(dims=["y", "x"], values=np.random.random([5, 10]))
    ds["b"] = sc.Variable(dims=["y", "x"],
                          values=np.random.random([5, 10]),
                          variances=np.random.random([5, 10]),
                          unit=sc.units.s)
    ds.masks["amask"] = sc.Variable(dims=["y"],
                                    values=[True, True, False, True, False])
    # Note that round trip would not work if attrs are present, since they get
    # become a per-item attribute during the conversion to dict.
    assert ds == sc.from_dict(sc.to_dict(ds))
