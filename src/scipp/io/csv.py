# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from io import BytesIO, StringIO
from os import PathLike
from typing import Union

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


def load_csv(
    filename: Union[str, PathLike[str], StringIO, BytesIO], sep: str = '\t'
) -> Dataset:
    df = _load_dataframe(filename, sep=sep)
    return from_pandas(df)
