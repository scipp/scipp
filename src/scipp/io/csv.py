# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

"""Load CSV files.

Note
----
CSV support requires `pandas <https://pandas.pydata.org/>`_ to be installed.
You need to do this manually as it is not declared as a dependency of Scipp.
You can use either ``pip install pandas`` or ``conda install -c conda-forge pandas``.


CSV ('comma separated values') files store a simple table of data as a string.
There are many different forms of this format.
So check whether the loaded data is what you expect for your files.
See :func:`scipp.io.csv.load_csv` for examples.

See Also
--------
pandas.read_csv:
    More details on the underlying parser.
"""

from collections.abc import Iterable
from io import BytesIO, StringIO
from os import PathLike
from typing import Any

from ..compat.pandas_compat import HeaderParserArg, from_pandas_dataframe
from ..core import Dataset


# The typehint of filepath_or_buffer is less generic than in pd.read_csv
# because the definitions of protocols are private in pandas.
def _load_dataframe(
    filepath_or_buffer: str | PathLike[str] | StringIO | BytesIO,
    sep: str | None,
    **kwargs: Any,
) -> Any:
    try:
        import pandas as pd
    except ImportError:
        raise ImportError(
            "Pandas is required to load CSV files but not install. "
            "Install it with `pip install pandas` or "
            "`conda install -c conda-forge pandas`."
        ) from None
    return pd.read_csv(filepath_or_buffer, sep=sep, **kwargs)


def load_csv(
    filename: str | PathLike[str] | StringIO | BytesIO,
    *,
    sep: str | None = ',',
    data_columns: str | Iterable[str] | None = None,
    header_parser: HeaderParserArg = None,
    **kwargs: Any,
) -> Dataset:
    """Load a CSV file as a dataset.

    This function currently uses Pandas to load the file and converts the result
    into a :class:`scipp.Dataset`.
    Pandas is not a hard dependency of Scipp and will thus not be installed
    automatically, so you need to install it manually.

    ``load_csv`` exists to conveniently load simple CSV files.
    If a file cannot be loaded directly, consider using Pandas directly.
    For example, use :func:`pandas.read_csv` to load the file into a data frame and
    :func:`scipp.compat.pandas_compat.from_pandas` to convert
    the data frame into a dataset.

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
    header_parser:
        Parser for column headers.
        See :func:`scipp.compat.pandas_compat.from_pandas` for details.
    **kwargs:
        Additional keyword arguments passed to :func:`pandas.read_csv`.

    Returns
    -------
    :
        The loaded data as a dataset.

    Examples
    --------
    Given the following CSV 'file':

       >>> from io import StringIO
       >>> csv_content = '''a [m],b [s],c
       ... 1,5,9
       ... 2,6,10
       ... 3,7,11
       ... 4,8,12'''

    By default, it will be loaded as

       >>> sc.io.load_csv(StringIO(csv_content))
       <scipp.Dataset>
       Dimensions: Sizes[row:4, ]
       Data:
         a [m]                       int64  [dimensionless]  (row)  [1, 2, 3, 4]
         b [s]                       int64  [dimensionless]  (row)  [5, 6, 7, 8]
         c                           int64  [dimensionless]  (row)  [9, 10, 11, 12]

    In this example, the column headers encode units.
    They can be parsed into actual units:

       >>> sc.io.load_csv(StringIO(csv_content), header_parser='bracket')
       <scipp.Dataset>
       Dimensions: Sizes[row:4, ]
       Data:
         a                           int64              [m]  (row)  [1, 2, 3, 4]
         b                           int64              [s]  (row)  [5, 6, 7, 8]
         c                           int64        <no unit>  (row)  [9, 10, 11, 12]

    It is possible to select which columns are stored as data:

       >>> sc.io.load_csv(
       ...     StringIO(csv_content),
       ...     header_parser='bracket',
       ...     data_columns='a',
       ... )
       <scipp.Dataset>
       Dimensions: Sizes[row:4, ]
       Coordinates:
       * b                           int64              [s]  (row)  [5, 6, 7, 8]
       * c                           int64        <no unit>  (row)  [9, 10, 11, 12]
       Data:
         a                           int64              [m]  (row)  [1, 2, 3, 4]
    """
    df = _load_dataframe(filename, sep=sep, **kwargs)
    return from_pandas_dataframe(
        df,
        data_columns=data_columns,
        include_trivial_index=False,
        header_parser=header_parser,
    )
