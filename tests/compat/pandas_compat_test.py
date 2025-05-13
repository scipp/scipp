# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Scipp contributors (https://github.com/scipp)

from collections.abc import Mapping

import numpy.typing as npt
import pytest

import scipp as sc
from scipp.compat import from_pandas
from scipp.compat.pandas_compat import parse_bracket_header
from scipp.testing import assert_identical

pandas = pytest.importorskip('pandas')


def _make_reference_da(
    row_name: str, values: npt.ArrayLike, dtype: str = "int64"
) -> sc.DataArray:
    return sc.DataArray(
        data=sc.array(dims=[row_name], values=values, dtype=dtype),
        coords={},
        name=row_name,
    )


def _make_1d_reference_ds(
    row_name: str, data_name: str, values: npt.ArrayLike, dtype: str = "int64"
) -> sc.Dataset:
    return sc.Dataset(
        data={data_name: _make_reference_da(row_name, values, dtype)},
    )


def _make_nd_reference_ds(
    row_name: str, data: Mapping[str, npt.ArrayLike], dtype: str = "int64"
) -> sc.Dataset:
    return sc.Dataset(
        data={
            key: _make_reference_da(row_name, value, dtype)
            for key, value in data.items()
        },
    )


def test_series() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_axis() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis("row-name", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row-name", [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_axis_non_str() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis(987, inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("987", [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.name = "the name"

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [1, 2, 3])
    reference_da.name = "the name"

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series_no_str() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.name = 8461

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [1, 2, 3])
    reference_da.name = "8461"

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series_and_named_axis() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis("axis-name", inplace=True)
    pd_df.name = "series-name"

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("axis-name", [1, 2, 3])
    reference_da.name = "series-name"

    assert sc.identical(sc_ds, reference_da)


def test_series_with_trivial_index_coord() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df, include_trivial_index=True)

    reference_da = _make_reference_da("row", [1, 2, 3])
    reference_da.coords["row"] = sc.arange("row", 3, dtype='int64')

    assert sc.identical(sc_ds, reference_da)


@pytest.mark.parametrize('include_trivial_index', [True, False])
def test_series_with_nontrivial_index_coord(include_trivial_index: bool) -> None:
    pd_df = pandas.Series(data=[1, 2, 3], index=[-1, -2, -3])

    sc_ds = from_pandas(pd_df, include_trivial_index=include_trivial_index)

    reference_da = _make_reference_da("row", [1, 2, 3])
    reference_da.coords["row"] = sc.arange("row", -1, -4, -1, dtype="int64")

    assert sc.identical(sc_ds, reference_da)


def test_series_without_name_parse_bracket_header() -> None:
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df, header_parser="bracket")

    reference_da = _make_reference_da("row", [1, 2, 3])
    reference_da.unit = None

    assert sc.identical(sc_ds, reference_da)


def test_1d_dataframe() -> None:
    pd_df = pandas.DataFrame(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_1d_reference_ds("row", "0", [1, 2, 3])

    assert sc.identical(sc_ds, reference_ds)


def test_1d_dataframe_with_named_axis() -> None:
    pd_df = pandas.DataFrame(data={"my-column": [1, 2, 3]})
    pd_df.rename_axis("1d_df", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_1d_reference_ds("1d_df", "my-column", [1, 2, 3])

    assert sc.identical(sc_ds, reference_ds)


def test_1d_dataframe_with_trivial_index_coord() -> None:
    pd_df = pandas.DataFrame(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df, include_trivial_index=True)

    reference_ds = _make_1d_reference_ds("row", "0", [1, 2, 3])
    reference_ds.coords["row"] = sc.arange("row", 3, dtype="int64")

    assert sc.identical(sc_ds, reference_ds)


@pytest.mark.parametrize('include_trivial_index', [True, False])
def test_1d_dataframe_with_nontrivial_index_coord(include_trivial_index: bool) -> None:
    pd_df = pandas.DataFrame(data=[1, 2, 3], index=[-1, -2, -3])

    sc_ds = from_pandas(pd_df, include_trivial_index=include_trivial_index)

    reference_ds = _make_1d_reference_ds("row", "0", [1, 2, 3])
    reference_ds.coords["row"] = sc.arange("row", -1, -4, -1, dtype="int64")

    assert sc.identical(sc_ds, reference_ds)


def test_2d_dataframe() -> None:
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_nd_reference_ds("row", data={"col1": (2, 3), "col2": (5, 6)})

    assert sc.identical(sc_ds, reference_ds)


def test_2d_dataframe_with_named_axes() -> None:
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})
    pd_df.rename_axis("my-name-for-rows", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_nd_reference_ds(
        "my-name-for-rows", data={"col1": (2, 3), "col2": (5, 6)}
    )

    assert sc.identical(sc_ds, reference_ds)


def test_dataframe_select_single_data() -> None:
    pd_df = pandas.DataFrame(data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)})

    sc_ds = from_pandas(pd_df, data_columns="col2")
    reference_ds = _make_nd_reference_ds(
        "row", data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)}
    )
    reference_ds.coords["col1"] = reference_ds.pop("col1").data
    reference_ds.coords["col3"] = reference_ds.pop("col3").data
    assert_identical(sc_ds, reference_ds)

    sc_ds = from_pandas(pd_df, data_columns=["col1"])
    reference_ds = _make_nd_reference_ds(
        "row", data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)}
    )
    reference_ds.coords["col2"] = reference_ds.pop("col2").data
    reference_ds.coords["col3"] = reference_ds.pop("col3").data
    assert_identical(sc_ds, reference_ds)


def test_dataframe_select_multiple_data() -> None:
    pd_df = pandas.DataFrame(data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)})

    sc_ds = from_pandas(pd_df, data_columns=["col3", "col1"])
    reference_ds = _make_nd_reference_ds(
        "row", data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)}
    )
    reference_ds.coords["col2"] = reference_ds.pop("col2").data
    assert_identical(sc_ds, reference_ds)


def test_dataframe_select_no_data() -> None:
    pd_df = pandas.DataFrame(data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)})

    sc_ds = from_pandas(pd_df, data_columns=[])
    reference_ds = _make_nd_reference_ds(
        "row", data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)}
    )
    reference_ds.coords["col1"] = reference_ds.pop("col1").data
    reference_ds.coords["col2"] = reference_ds.pop("col2").data
    reference_ds.coords["col3"] = reference_ds.pop("col3").data
    assert_identical(sc_ds, reference_ds)


def test_dataframe_select_undefined_raises() -> None:
    pd_df = pandas.DataFrame(data={"col1": (1, 2), "col2": (6, 3), "col3": (4, 0)})

    with pytest.raises(KeyError):
        _ = from_pandas(pd_df, data_columns=["unknown"])


def test_2d_dataframe_does_not_parse_units_by_default() -> None:
    pd_df = pandas.DataFrame(data={"col1 [m]": (1, 2), "col2 [one]": (6, 3)})

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_nd_reference_ds(
        "row", data={"col1 [m]": (1, 2), "col2 [one]": (6, 3)}
    )

    assert_identical(sc_ds, reference_ds)


def test_2d_dataframe_parse_units_brackets() -> None:
    pd_df = pandas.DataFrame(data={"col1 [m]": (1, 2), "col2 [one]": (6, 3)})

    sc_ds = from_pandas(pd_df, header_parser="bracket")

    reference_ds = _make_nd_reference_ds("row", data={"col1": (1, 2), "col2": (6, 3)})
    reference_ds["col1"].unit = "m"  # type: ignore[assignment]
    reference_ds["col2"].unit = "one"  # type: ignore[assignment]

    assert_identical(sc_ds, reference_ds)


def test_2d_dataframe_parse_units_brackets_string_dtype() -> None:
    pd_df = pandas.DataFrame(
        data={"col1 [m]": ("a", "b"), "col2": ("c", "d")}, dtype="string"
    )

    sc_ds = from_pandas(pd_df, header_parser="bracket")

    reference_ds = _make_nd_reference_ds(
        "row",
        data={"col1": ("a", "b"), "col2": ("c", "d")},
        dtype="str",
    )
    reference_ds["col1"].unit = "m"  # type: ignore[assignment]
    reference_ds["col2"].unit = None

    assert_identical(sc_ds, reference_ds)


def test_parse_bracket_header_whitespace() -> None:
    name, unit = parse_bracket_header("")
    assert name == ""
    assert unit is None

    name, unit = parse_bracket_header(" ")
    assert name == " "
    assert unit is None


def test_parse_bracket_header_only_name() -> None:
    name, unit = parse_bracket_header("a name 123")
    assert name == "a name 123"
    assert unit is None

    name, unit = parse_bracket_header(" padded name  ")
    assert name == " padded name  "
    assert unit is None


def test_parse_bracket_header_only_unit() -> None:
    name, unit = parse_bracket_header("[m]")
    assert name == ""
    assert unit == "m"

    name, unit = parse_bracket_header(" [kg]")
    assert name == ""
    assert unit == "kg"


def test_parse_bracket_header_name_and_unit() -> None:
    name, unit = parse_bracket_header("the name [s]")
    assert name == "the name"
    assert unit == "s"

    name, unit = parse_bracket_header("title[A]")
    assert name == "title"
    assert unit == "A"


def test_parse_bracket_header_empty_unit() -> None:
    name, unit = parse_bracket_header("name []")
    assert name == "name"
    assert unit == sc.units.default_unit


def test_parse_bracket_header_dimensionless() -> None:
    name, unit = parse_bracket_header("name [one]")
    assert name == "name"
    assert unit == "one"

    name, unit = parse_bracket_header("name [dimensionless]")
    assert name == "name"
    assert unit == "one"


def test_parse_bracket_header_complex_unit() -> None:
    name, unit = parse_bracket_header("name [m / s**2]")
    assert name == "name"
    assert unit == "m/s**2"


def test_parse_bracket_header_bad_string() -> None:
    name, unit = parse_bracket_header("too [many] [brackets]")
    assert name == "too [many] [brackets]"
    assert unit is None


def test_parse_bracket_header_bad_unit() -> None:
    name, unit = parse_bracket_header("label [bogus]")
    assert name == "label [bogus]"
    assert unit is None
