import pandas
import scipp as sc
from scipp.compat import from_pandas


def _make_reference_da(row_name, row_coords, values, dtype="int64"):
    return sc.DataArray(
        data=sc.Variable(dims=[row_name], values=values, dtype=dtype),
        coords={row_name: sc.Variable(dims=[row_name], values=row_coords, dtype=dtype)},
        name=row_name,
    )


def _make_1d_reference_ds(row_name, data_name, values, coords, dtype="int64"):
    return sc.Dataset(
        data={data_name: sc.Variable(dims=[row_name], values=values, dtype=dtype)},
        coords={row_name: sc.Variable(dims=[row_name], values=coords, dtype=dtype)},
    )


def _make_2d_reference_ds(row_name, row_coords, data, dtype="int64"):
    return sc.Dataset(
        data={
            key: sc.Variable(dims=[row_name], values=value, dtype=dtype)
            for key, value in data.items()
        },
        coords={
            row_name: sc.Variable(dims=[row_name], values=row_coords, dtype=dtype),
        },
    )


def test_series():
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [0, 1, 2], [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_axis():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis("row-name", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row-name", [0, 1, 2], [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_axis_non_str():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis(987, inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("987", [0, 1, 2], [1, 2, 3])

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.name = "the name"

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [0, 1, 2], [1, 2, 3])
    reference_da.name = "the name"

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series_no_str():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.name = 8461

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("row", [0, 1, 2], [1, 2, 3])
    reference_da.name = "8461"

    assert sc.identical(sc_ds, reference_da)


def test_series_with_named_series_and_named_axis():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis("axis-name", inplace=True)
    pd_df.name = "series-name"

    sc_ds = from_pandas(pd_df)

    reference_da = _make_reference_da("axis-name", [0, 1, 2], [1, 2, 3])
    reference_da.name = "series-name"

    assert sc.identical(sc_ds, reference_da)


def test_1d_dataframe():
    pd_df = pandas.DataFrame(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_1d_reference_ds("row", "0", [1, 2, 3], [0, 1, 2])

    assert sc.identical(sc_ds, reference_ds)


def test_1d_dataframe_with_named_axis():
    pd_df = pandas.DataFrame(data={"my-column": [1, 2, 3]})
    pd_df.rename_axis("1d_df", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_1d_reference_ds("1d_df", "my-column", [1, 2, 3], [0, 1, 2])

    assert sc.identical(sc_ds, reference_ds)


def test_2d_dataframe():
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_2d_reference_ds("row", [0, 1],
                                         data={
                                             "col1": (2, 3),
                                             "col2": (5, 6)
                                         })

    assert sc.identical(sc_ds, reference_ds)


def test_2d_dataframe_with_named_axes():
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})
    pd_df.rename_axis("my-name-for-rows", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_ds = _make_2d_reference_ds("my-name-for-rows", [0, 1],
                                         data={
                                             "col1": (2, 3),
                                             "col2": (5, 6)
                                         })

    assert sc.identical(sc_ds, reference_ds)
