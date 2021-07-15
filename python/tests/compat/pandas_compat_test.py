import pandas
import scipp as sc
from scipp.compat.pandas_compat import from_pandas


def _make_1d_reference_ds(row_name, data_name, values, coords, dtype="int64"):
    return sc.Dataset(
        data={
            data_name: sc.Variable(dims=[row_name], values=values, dtype=dtype)
        },
        coords={
            row_name: sc.Variable(dims=[row_name], values=coords, dtype=dtype)
        },
    )


def _make_2d_reference_ds(row_name, row_coords, data, dtype="int64"):

    return sc.Dataset(
        data={
            key: sc.Variable(dims=[row_name], values=value, dtype=dtype)
            for key, value in data.items()
        },
        coords={
            row_name: sc.Variable(dims=[row_name],
                                  values=row_coords,
                                  dtype=dtype),
        },
    )


def test_1d_dataframe():
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_ds = from_pandas(pd_df)

    reference_da = _make_1d_reference_ds("row", "", [1, 2, 3], [0, 1, 2])

    assert sc.identical(sc_ds, reference_da)


def test_1d_dataframe_with_named_axis():
    pd_df = pandas.Series(data=[1, 2, 3], name="series1")

    sc_ds = from_pandas(pd_df)

    reference_da = _make_1d_reference_ds("row", "series1", [1, 2, 3],
                                         [0, 1, 2])

    assert sc.identical(sc_ds, reference_da)


def test_2d_dataframe():
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})

    sc_ds = from_pandas(pd_df)

    reference_da = _make_2d_reference_ds("row", [0, 1],
                                         data={
                                             "col1": (2, 3),
                                             "col2": (5, 6)
                                         })

    assert sc.identical(sc_ds, reference_da)


def test_2d_dataframe_with_named_axes():
    pd_df = pandas.DataFrame(data={"col1": (2, 3), "col2": (5, 6)})
    pd_df.rename_axis("my-name-for-rows", inplace=True)

    sc_ds = from_pandas(pd_df)

    reference_da = _make_2d_reference_ds("my-name-for-rows", [0, 1],
                                         data={
                                             "col1": (2, 3),
                                             "col2": (5, 6)
                                         })

    assert sc.identical(sc_ds, reference_da)


# def test_attrs():
#     pd_df = pandas.DataFrame(data={})
#     pd_df.attrs = {
#         "attrib_int": 5,
#         "attrib_float": 6.54321,
#         "attrib_str": "test-string",
#     }
#
#     sc_da = from_pandas(pd_df)
#
#     assert sc_da.attrs["attrib_int"].values == 5
#     assert sc_da.attrs["attrib_float"].values == 6.54321
#     assert sc_da.attrs["attrib_str"].values == "test-string"
