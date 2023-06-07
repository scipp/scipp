# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from io import BytesIO, StringIO
from os import PathLike
from typing import Iterable, Optional, Union

from ..compat import from_pandas
from ..core import Dataset


# The typehint of filepath_or_buffer is only an approximation of Panda's
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


def _assign_coords_in_place(
    ds: Dataset, data_columns: Optional[Union[str, Iterable[str]]] = None
):
    if data_columns is None:
        return
    if isinstance(data_columns, str):
        data_columns = {data_columns}

    if extra_keys := set(data_columns) - set(ds.keys()):
        raise KeyError(f"Dataset has no such columns: {extra_keys}")

    for key in list(ds.keys()):
        if key not in data_columns:
            ds.coords[key] = ds.pop(key).data


def load_csv(
    filename: Union[str, PathLike[str], StringIO, BytesIO],
    *,
    sep: str = '\t',
    data_columns: Optional[Union[str, Iterable[str]]] = None,
    include_index: bool = True,
) -> Dataset:
    df = _load_dataframe(filename, sep=sep)
    ds = from_pandas(df, include_index=include_index)
    _assign_coords_in_place(ds, data_columns)
    return ds
