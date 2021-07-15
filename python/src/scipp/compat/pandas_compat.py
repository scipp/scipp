from __future__ import annotations

from typing import Union, TYPE_CHECKING

from .. import Dataset, DataArray, Variable

if TYPE_CHECKING:
    import pandas as pd


def from_pandas(df: Union[pd.DataFrame, pd.Series]) -> DataArray:
    """
    Converts a pandas.DataFrame or pandas.Series object into a
    scipp DataArray.

    :param df: the Dataframe to convert
    :return: the converted scipp object.
    """

    row_index = df.axes[0]
    rows_index_name = row_index.name or "row"

    sc_data = {}

    if df.ndim == 2:
        column_names = df.axes[1]
    else:
        column_names = [df.name]

    for column_name in column_names:
        if column_name is None:
            if df.ndim != 1:
                raise ValueError("from_pandas: got unnamed column "
                                 "in dataframe with multiple columns")
            column_name = ""

        sc_data[column_name] = DataArray(data=Variable(
            values=df[column_name].values if df.ndim == 2 else df.values,
            dims=[rows_index_name]),
                                         coords={
                                             rows_index_name:
                                             Variable(dims=[rows_index_name],
                                                      values=row_index)
                                         },
                                         name=column_name or "")

    return Dataset(data=sc_data,
                   coords={
                       rows_index_name:
                       Variable(dims=[rows_index_name], values=row_index)
                   })
