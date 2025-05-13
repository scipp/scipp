# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from typing import Any

import numpy as np

import scipp as sc


def test_variable_to_dict() -> None:
    var = sc.Variable(
        dims=["x"],
        values=np.arange(10.0),
        variances=np.random.random(10),
        unit=sc.units.m,
    )
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == var.dims
    assert var_dict["shape"] == var.shape
    assert np.array_equal(var_dict["values"], var.values)
    assert np.array_equal(var_dict["variances"], var.variances)
    assert var_dict["unit"] == var.unit
    assert var_dict["dtype"] == var.dtype


def test_variable_0D_to_dict() -> None:
    var = 12.0 * sc.units.one
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert var_dict["values"] == 12.0


def test_variable_vector_to_dict() -> None:
    var = sc.vectors(dims=['x'], values=np.random.random([10, 3]))
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ('x',)
    assert var_dict["shape"] == (10,)
    assert var_dict["values"].shape == (10, 3)
    assert var_dict["dtype"] == sc.DType.vector3


def test_variable_0D_vector_to_dict() -> None:
    var = sc.vector(value=[1, 2, 3])
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert np.array_equal(var_dict["values"], [1, 2, 3])
    assert var_dict["dtype"] == sc.DType.vector3


def test_variable_linear_transform_to_dict() -> None:
    data = np.array(
        [
            np.arange(9.0).reshape(3, 3),
            np.arange(5.0, 14.0).reshape(3, 3),
            np.arange(1.0, 10.0).reshape(3, 3),
            np.arange(2.0, 11.0).reshape(3, 3),
        ]
    )
    var = sc.spatial.linear_transforms(dims=['x'], values=data, unit=sc.units.m)
    var_dict = sc.to_dict(var)
    assert var_dict["shape"] == (4,)
    assert var_dict["values"].shape == (4, 3, 3)
    assert var_dict["dtype"] == sc.DType.linear_transform3


def test_variable_0D_linear_transform_to_dict() -> None:
    var = sc.spatial.linear_transform(value=np.arange(1, 10).reshape(3, 3))
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert np.array_equal(var_dict["values"], [[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert var_dict["dtype"] == sc.DType.linear_transform3


def test_variable_from_dict() -> None:
    var_dict = {
        "dims": ('x',),
        "values": np.random.random(10),
        "variances": np.random.random(10),
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == (10,)
    assert np.array_equal(var.values, var_dict["values"])  # type: ignore[arg-type]
    assert np.array_equal(var.variances, var_dict["variances"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one


def test_variable_0D_from_dict() -> None:
    var_dict = {"dims": (), "values": 17.0, "variances": 0.2}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert var.value == 17.0
    assert var.variance == 0.2
    assert var.unit == sc.units.one


def test_variable_vector_from_dict() -> None:
    var_dict = {
        "dims": ('x',),
        "values": np.arange(6).reshape(2, 3),
        "dtype": "vector3",
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == (2,)
    assert np.array_equal(np.array(var.values), [[0, 1, 2], [3, 4, 5]])
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.vector3


def test_variable_0D_vector_from_dict() -> None:
    var_dict = {"dims": (), "values": [1, 2, 3], "dtype": "vector3"}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert np.array_equal(np.array(var.values), [1, 2, 3])
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.vector3


def test_variable_linear_transform_from_dict() -> None:
    var_dict = {
        "dims": ('x',),
        "values": np.arange(18).reshape(2, 3, 3),
        "dtype": "linear_transform3",
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == (2,)
    assert np.array_equal(np.array(var.values), var_dict["values"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.linear_transform3


def test_variable_0D_linear_transform_from_dict() -> None:
    var_dict = {
        "dims": (),
        "values": np.arange(9).reshape(3, 3),
        "dtype": "linear_transform3",
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert np.array_equal(np.array(var.value), var_dict["values"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.linear_transform3


def test_variable_round_trip() -> None:
    var = sc.Variable(
        dims=["x"],
        values=np.arange(10.0),
        variances=np.random.random(10),
        unit=sc.units.m,
    )
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_0D_round_trip() -> None:
    var = 12.0 * sc.units.one
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_vector_round_trip() -> None:
    var = sc.vectors(dims=['x'], values=np.random.random([10, 3]))
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_0D_vector_round_trip() -> None:
    var = sc.vector(value=[1, 2, 3])
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_linear_transform_round_trip() -> None:
    data = np.array(
        [
            np.arange(9.0).reshape(3, 3),
            np.arange(5.0, 14.0).reshape(3, 3),
            np.arange(1.0, 10.0).reshape(3, 3),
            np.arange(2.0, 11.0).reshape(3, 3),
        ]
    )
    var = sc.spatial.linear_transforms(dims=['x'], values=data, unit=sc.units.m)
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_0D_matrix_round_trip() -> None:
    var = sc.spatial.linear_transform(value=np.arange(1, 10).reshape(3, 3))
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_data_array_to_dict() -> None:
    da = sc.DataArray(
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10.0)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
        masks={
            "amask": sc.Variable(dims=["y"], values=[True, True, False, True, False])
        },
        data=sc.Variable(dims=["y", "x"], values=np.random.random([5, 10])),
    )
    da.coords.set_aligned("y", False)
    da_dict = sc.to_dict(da)
    assert da_dict["data"]["dims"] == da.dims
    assert da_dict["data"]["shape"] == da.shape
    assert np.array_equal(da_dict["data"]["values"], da.values)
    assert np.array_equal(da_dict["data"]["variances"], da.variances)
    assert da_dict["data"]["unit"] == da.unit
    assert da_dict["data"]["dtype"] == da.dtype
    assert sc.identical(sc.from_dict(da_dict["coords"]["x"]), da.coords["x"])
    assert sc.identical(sc.from_dict(da_dict["coords"]["y"]), da.coords["y"])
    assert not da_dict["coords"]["y"]["aligned"]
    assert sc.identical(sc.from_dict(da_dict["masks"]["amask"]), da.masks["amask"])


def test_data_array_from_dict() -> None:
    da_dict: dict[str, Any] = {
        "coords": {
            "x": {"dims": ["x"], "values": np.arange(10), "aligned": False},
            "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
        },
        "masks": {"amask": {"dims": ["y"], "values": [True, True, False, True, False]}},
        "data": {"dims": ["y", "x"], "values": np.random.random([5, 10])},
    }
    da = sc.from_dict(da_dict)
    assert isinstance(da, sc.DataArray)
    assert sc.identical(da.coords["x"], sc.from_dict(da_dict["coords"]["x"]))
    assert not da.coords["x"].aligned
    assert sc.identical(da.coords["y"], sc.from_dict(da_dict["coords"]["y"]))
    assert da.coords["y"].aligned
    assert sc.identical(da.masks["amask"], sc.from_dict(da_dict["masks"]["amask"]))
    assert sc.identical(da.data, sc.from_dict(da_dict["data"]))


def test_data_array_round_trip() -> None:
    da = sc.DataArray(
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10.0)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
        masks={
            "amask": sc.Variable(dims=["y"], values=[True, True, False, True, False])
        },
        data=sc.Variable(dims=["y", "x"], values=np.random.random([5, 10])),
    )
    assert sc.identical(da, sc.from_dict(sc.to_dict(da)))


def test_dataset_to_dict() -> None:
    ds = sc.Dataset(
        {
            'a': sc.array(dims=["y", "x"], values=np.random.random([5, 10])),
            "b": sc.array(
                dims=["y", "x"],
                values=np.random.random([5, 10]),
                variances=np.random.random([5, 10]),
                unit=sc.units.s,
            ),
        },
        coords={
            "x": sc.array(dims=["x"], values=np.arange(10)),
            "y": sc.array(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
    )
    ds["a"].masks["amask"] = sc.array(
        dims=["y"], values=[True, True, False, True, False]
    )
    # Note that attributes complicate things here, as they are duplicated in
    # each entry during the conversion to dict. So for now, we leave attributes
    # out.
    ds_dict = sc.to_dict(ds)
    assert sc.identical(sc.from_dict(ds_dict["a"]), ds["a"])
    assert sc.identical(sc.from_dict(ds_dict["b"]), ds["b"])


def test_dataset_from_dict() -> None:
    ds_dict = {
        "a": {
            "coords": {
                "x": {"dims": ["x"], "values": np.arange(10)},
                "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
            },
            "masks": {
                "amask": {"dims": ["y"], "values": [True, True, False, True, False]}
            },
            "data": {"dims": ["y", "x"], "values": np.random.random([5, 10])},
        },
        "b": {
            "coords": {
                "x": {"dims": ["x"], "values": np.arange(10)},
                "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
            },
            "masks": {
                "amask": {"dims": ["y"], "values": [True, True, False, True, False]}
            },
            "data": {
                "dims": ["y", "x"],
                "values": np.random.random([5, 10]),
                "variances": np.random.random([5, 10]),
            },
        },
    }
    ds = sc.from_dict(ds_dict)
    assert isinstance(ds, sc.Dataset)
    assert sc.identical(ds["a"], sc.from_dict(ds_dict["a"]))
    assert sc.identical(ds["b"], sc.from_dict(ds_dict["b"]))


def test_dataset_round_trip() -> None:
    ds = sc.Dataset(
        {
            "a": sc.array(dims=["y", "x"], values=np.random.random([5, 10])),
            "b": sc.array(
                dims=["y", "x"],
                values=np.random.random([5, 10]),
                variances=np.random.random([5, 10]),
                unit=sc.units.s,
            ),
        },
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
    )
    ds["a"].masks["amask"] = sc.Variable(
        dims=["y"], values=[True, True, False, True, False]
    )
    # Note that round trip would not work if attrs are present, since they get
    # become a per-item attribute during the conversion to dict.
    assert sc.identical(ds, sc.from_dict(sc.to_dict(ds)))
