# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from io import BytesIO, StringIO
from os import PathLike
from typing import Iterable, Optional, Union

from ..compat.pandas_compat import HeadParserArg, from_pandas
from ..core import Dataset


# The typehint of filepath_or_buffer is less generic than in pd.read_table
# because the definitions of protocols are private in pandas.
def _load_dataframe(
    filepath_or_buffer: Union[str, PathLike[str], StringIO, BytesIO], sep: str
):
    try:
        import pandas as pd
    except ImportError:
        raise ImportError(
            "Pandas is required to load CSV files but not install. "
            "Install it with `pip install pandas` or "
            "`conda install -c conda-forge pandas`."
        ) from None
    return pd.read_table(filepath_or_buffer, sep=sep)


def load_csv(
    filename: Union[str, PathLike[str], StringIO, BytesIO],
    *,
    sep: Optional[str] = '\t',
    data_columns: Optional[Union[str, Iterable[str]]] = None,
    include_index: bool = True,
    head_parser: HeadParserArg = None,
) -> Dataset:
    """Load a CSV file as a dataset.

    Parameters
    ----------
    filename:
        Path or URL of file to load or buffer to load from.
    sep:
        Column separator.
        Automatically deduced if ``sep is None``.
        See :func:`pandas.read_csv` for details.
    data_columns:
        Select which columns to assign as data.
        The rest are returned as coordinates.
        If ``None``, all columns are assigned as data.
        Use an empty list to assign all columns as coordinates.
    include_index:
        If ``True``, include the index as a coordinate.
    head_parser:
        Parser for column headers.
        See :func:`scipp.compat.pandas_compat.from_pandas` for details.
    """
    df = _load_dataframe(filename, sep=sep)
    return from_pandas(
        df,
        data_columns=data_columns,
        include_index=include_index,
        head_parser=head_parser,
    )
