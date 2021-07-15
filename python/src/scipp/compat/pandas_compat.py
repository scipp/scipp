from __future__ import annotations

from typing import Union, TYPE_CHECKING

from .. import DataArray, scalar, Variable

if TYPE_CHECKING:
    import pandas as pd


def from_pandas(df: Union[pd.DataFrame, pd.Series]) -> DataArray:
    """
    Converts a pandas.DataFrame or pandas.Series object into a
    scipp DataArray.

    :param df: the Dataframe to convert
    :return: the converted scipp object.
    """
    sc_attribs = {}

    for attr in df.attrs:
        sc_attribs[attr] = scalar(df.attrs[attr])

    row_index = df.axes[0]
    rows_index_name = row_index.name or "row"

    sc_dims = [rows_index_name]
    sc_coords = {
        rows_index_name: Variable(
            dims=[rows_index_name],
            values=row_index,
        )
    }

    if df.ndim == 2:
        column_index = df.axes[1]
        column_index_name = column_index.name or "column"

        sc_dims.append(column_index_name)
        sc_coords[column_index_name] = Variable(
            dims=[column_index_name],
            values=column_index,
        )

    return DataArray(
        data=Variable(values=df.to_numpy(), dims=sc_dims),
        coords=sc_coords,
        attrs=sc_attribs,
    )
