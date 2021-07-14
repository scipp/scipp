import pandas

import scipp
import typing


def from_pandas(
    dataframe: typing.Union[pandas.DataFrame,
                            pandas.Series]) -> scipp.DataArray:
    sc_attribs = {}

    for attr in dataframe.attrs:
        sc_attribs[attr] = scipp.scalar(dataframe.attrs[attr])

    row_index = dataframe.axes[0]
    rows_index_name = row_index.name or "pandas_row"

    sc_dims = [rows_index_name]
    sc_coords = {
        rows_index_name: scipp.Variable(
            dims=[rows_index_name],
            values=row_index,
        )
    }

    if dataframe.ndim == 2:
        column_index = dataframe.axes[1]
        column_index_name = column_index.name or "pandas_column"

        sc_dims.append(column_index_name)
        sc_coords[column_index_name] = scipp.Variable(
            dims=[column_index_name],
            values=column_index,
        )

    return scipp.DataArray(
        data=scipp.Variable(values=dataframe.to_numpy(), dims=sc_dims),
        coords=sc_coords,
        attrs=sc_attribs,
    )
