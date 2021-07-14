import pandas
import scipp
from scipp.compat.pandas_compat import from_pandas


def test_1d_dataframe():
    pandas_df = pandas.Series(data=[1, 2, 3])

    sc_dataarray = from_pandas(pandas_df)

    assert (sc_dataarray.values == scipp.array(dims=["pandas_row"],
                                               values=[1, 2, 3]).values).all()
    assert sc_dataarray.dims == ["pandas_row"]


def test_1d_dataframe_with_named_axis():
    pandas_df = pandas.Series(data=[1, 2, 3])
    pandas_df.rename_axis("counting-to-three", inplace=True)

    sc_dataarray = from_pandas(pandas_df)

    assert (sc_dataarray.values == scipp.array(dims=["counting-to-three"],
                                               values=[1, 2, 3]).values).all()
    assert sc_dataarray.dims == ["counting-to-three"]


def test_2d_dataframe():
    pandas_df = pandas.DataFrame(data={1: (2, 3), 4: (5, 6)})

    sc_dataarray = from_pandas(pandas_df)

    assert (sc_dataarray.values == scipp.array(
        dims=["pandas_row", "pandas_column"], values=[(2, 5),
                                                      (3, 6)]).values).all()
    assert sc_dataarray.dims == ["pandas_row", "pandas_column"]


def test_2d_dataframe_with_named_axes():
    pandas_df = pandas.DataFrame(data={1: (2, 3), 4: (5, 6)})
    pandas_df.rename_axis("my-name-for-rows", inplace=True)
    pandas_df.rename_axis("my-name-for-columns", axis="columns", inplace=True)

    sc_dataarray = from_pandas(pandas_df)

    assert (sc_dataarray.values == scipp.array(
        dims=["my-name-for-rows", "my-name-for-columns"],
        values=[(2, 5), (3, 6)]).values).all()
    assert sc_dataarray.dims == ["my-name-for-rows", "my-name-for-columns"]


def test_attrs():
    pandas_df = pandas.DataFrame(data={})
    pandas_df.attrs = {
        "attrib_int": 5,
        "attrib_float": 6.54321,
        "attrib_str": "test-string",
    }

    sc_dataarray = from_pandas(pandas_df)

    assert sc_dataarray.attrs["attrib_int"].values == 5
    assert sc_dataarray.attrs["attrib_float"].values == 6.54321
    assert sc_dataarray.attrs["attrib_str"].values == "test-string"
