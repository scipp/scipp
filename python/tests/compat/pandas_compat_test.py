import pandas
import scipp as sc
from scipp.compat.pandas_compat import from_pandas


def _make_1d_reference_da(row_name, values, coords, dtype="int64"):
    return sc.DataArray(
        data=sc.Variable(dims=[row_name], values=values, dtype=dtype),
        coords={
            row_name: sc.Variable(dims=[row_name], values=coords, dtype=dtype)
        },
    )


def test_1d_dataframe():
    pd_df = pandas.Series(data=[1, 2, 3])

    sc_da = from_pandas(pd_df)

    reference_da = _make_1d_reference_da("row", [1, 2, 3], [0, 1, 2])

    assert sc.identical(sc_da, reference_da)


def test_1d_dataframe_with_named_axis():
    pd_df = pandas.Series(data=[1, 2, 3])
    pd_df.rename_axis("counting-to-three", inplace=True)

    sc_da = from_pandas(pd_df)

    reference_da = _make_1d_reference_da("counting-to-three", [1, 2, 3],
                                         [0, 1, 2])

    assert sc.identical(sc_da, reference_da)


# def test_2d_dataframe():
#     pd_df = pandas.DataFrame(data={1: (2, 3), 4: (5, 6)})
#
#     sc_da = from_pandas(pd_df)
#
#     assert sc.identical(sc_da,
#         sc.DataArray(dims=["row", "column"], data=[(2, 5), (3, 6)]))
#
#
# def test_2d_dataframe_with_named_axes():
#     pd_df = pandas.DataFrame(data={1: (2, 3), 4: (5, 6)})
#     pd_df.rename_axis("my-name-for-rows", inplace=True)
#     pd_df.rename_axis("my-name-for-columns", axis="columns", inplace=True)
#
#     sc_da = from_pandas(pd_df)
#
#     assert sc.identical(sc_da, sc.DataArray(
#         dims=["my-name-for-rows", "my-name-for-columns"],
#         values=[(2, 5), (3, 6)]))


def test_attrs():
    pd_df = pandas.DataFrame(data={})
    pd_df.attrs = {
        "attrib_int": 5,
        "attrib_float": 6.54321,
        "attrib_str": "test-string",
    }

    sc_da = from_pandas(pd_df)

    assert sc_da.attrs["attrib_int"].values == 5
    assert sc_da.attrs["attrib_float"].values == 6.54321
    assert sc_da.attrs["attrib_str"].values == "test-string"
