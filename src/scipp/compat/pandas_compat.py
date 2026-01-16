# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from collections.abc import Callable, Iterable
from typing import TYPE_CHECKING, Any, Literal, overload

from ..core import DataArray, Dataset, Unit, UnitError, array
from ..units import default_unit

if TYPE_CHECKING:
    import pandas as pd


def _index_is_trivial(index: pd.Index[int], n_rows: int) -> bool:
    from pandas import RangeIndex

    return (
        isinstance(index, RangeIndex)
        and index.start == 0
        and index.stop == n_rows
        and index.step == 1
    )


def from_pandas_series(
    se: pd.Series[Any],
    *,
    include_trivial_index: bool = False,
    header_parser: HeaderParserArg = None,
) -> DataArray:
    row_index = se.axes[0]
    row_index_name = "row" if row_index.name is None else str(row_index.name)
    name, unit = _parse_header("" if se.name is None else str(se.name), header_parser)

    coords = (
        {row_index_name: array(dims=[row_index_name], values=row_index)}
        if include_trivial_index or not _index_is_trivial(row_index, len(se))
        else {}
    )

    if se.dtype == "string":
        # se.to_numpy() and np.array(se.values) produce an array of dtype=object
        # when the series contains strings.
        values = se.to_numpy(dtype=str)
    else:
        values = se.to_numpy()
    return DataArray(
        data=array(values=values, dims=[row_index_name], unit=unit),
        coords=coords,
        name=name,
    )


def from_pandas_dataframe(
    df: pd.DataFrame,
    *,
    data_columns: str | Iterable[str] | None = None,
    include_trivial_index: bool = False,
    header_parser: HeaderParserArg = None,
) -> Dataset:
    import pandas as pd

    columns = (
        from_pandas_series(
            pd.Series(df[column_name]),
            include_trivial_index=include_trivial_index,
            header_parser=header_parser,
        )
        for column_name in df.axes[1]
    )
    data_arrays = {da.name: da for da in columns}

    if data_columns is None:
        data = data_arrays
        coords = {}
    else:
        if isinstance(data_columns, str):
            data_columns = (data_columns,)
        data = {name: data_arrays.pop(name) for name in data_columns}
        coords = {name: coord.data for name, coord in data_arrays.items()}

    return Dataset(data, coords=coords)


@overload
def from_pandas(
    pd_obj: pd.DataFrame,
    *,
    data_columns: str | Iterable[str] | None = None,
    include_trivial_index: bool = False,
    header_parser: HeaderParserArg = None,
) -> Dataset: ...


@overload
def from_pandas(
    pd_obj: pd.Series[Any],
    *,
    data_columns: str | Iterable[str] | None = None,
    include_trivial_index: bool = False,
    header_parser: HeaderParserArg = None,
) -> DataArray: ...


def from_pandas(
    pd_obj: pd.DataFrame | pd.Series[Any],
    *,
    data_columns: str | Iterable[str] | None = None,
    include_trivial_index: bool = False,
    header_parser: HeaderParserArg = None,
) -> DataArray | Dataset:
    """Converts a pandas.DataFrame or pandas.Series object into a
    scipp Dataset or DataArray respectively.

    Parameters
    ----------
    pd_obj:
        The Dataframe or Series to convert.
    data_columns:
        Select which columns to assign as data.
        The rest are returned as coordinates.
        If ``None``, all columns are assigned as data.
        Use an empty list to assign all columns as coordinates.
    include_trivial_index:
        ``from_pandas`` can include the index of the data frame / series as a
        coordinate.
        But when the index is ``RangeIndex(start=0, stop=n, step=1)``, where ``n``
        is the length of the data frame / series, the index is excluded by default.
        Set this argument to ``True`` to include to index anyway in this case.
    header_parser:
        Parses each column header to extract a name and unit for each data array.
        By default, it returns the column name and uses the default unit.
        Builtin parsers can be specified by name:

        - ``"bracket"``: See :func:`scipp.compat.pandas_compat.parse_bracket_header`.
          Parses strings where the unit is given between square brackets,
          i.e., strings like ``name [unit]``.

        Before implementing a custom parser, check out
        :func:`scipp.compat.pandas_compat.parse_bracket_header`
        to get an overview of how to handle edge cases.

    Returns
    -------
    :
        The converted scipp object.

    Examples
    --------
    Convert a pandas Series to a DataArray:

      >>> import scipp as sc
      >>> import pandas as pd
      >>> series = pd.Series([1.0, 2.0, 3.0], name='temperature [K]')
      >>> sc.compat.from_pandas(series, header_parser='bracket')
      <scipp.DataArray>
      Dimensions: Sizes[row:3, ]
      Data:
        temperature               float64              [K]  (row)  [1, 2, 3]

    Convert a pandas DataFrame to a Dataset, with all columns as data:

      >>> df = pd.DataFrame({
      ...     'x [m]': [1.0, 2.0, 3.0],
      ...     'y [m]': [4.0, 5.0, 6.0],
      ...     'temperature [K]': [273.0, 274.0, 275.0]
      ... })
      >>> ds = sc.compat.from_pandas(df, header_parser='bracket')
      >>> ds
      <scipp.Dataset>
      Dimensions: Sizes[row:3, ]
      Data:
        temperature               float64              [K]  (row)  [273, 274, 275]
        x                         float64              [m]  (row)  [1, 2, 3]
        y                         float64              [m]  (row)  [4, 5, 6]

    Specify which columns should be data vs coordinates:

      >>> ds = sc.compat.from_pandas(df, data_columns='temperature', header_parser='bracket')
      >>> ds
      <scipp.Dataset>
      Dimensions: Sizes[row:3, ]
      Coordinates:
      * x                         float64              [m]  (row)  [1, 2, 3]
      * y                         float64              [m]  (row)  [4, 5, 6]
      Data:
        temperature               float64              [K]  (row)  [273, 274, 275]
    """  # noqa: E501
    import pandas as pd

    if isinstance(pd_obj, pd.DataFrame):
        return from_pandas_dataframe(
            pd_obj,
            data_columns=data_columns,
            include_trivial_index=include_trivial_index,
            header_parser=header_parser,
        )
    elif isinstance(pd_obj, pd.Series):
        return from_pandas_series(
            pd_obj,
            include_trivial_index=include_trivial_index,
            header_parser=header_parser,
        )
    else:
        raise ValueError(f"from_pandas: cannot convert type '{type(pd_obj)}'")


HeaderParser = Callable[[str], tuple[str, Unit | None]]
HeaderParserArg = Literal["bracket"] | HeaderParser | None


def parse_bracket_header(head: str) -> tuple[str, Unit | None]:
    """Parses strings of the form ``name [unit]``.

    ``name`` may be any string that does not contain the character ``[``.
    And ``unit`` must be a valid unit string to be parsed by ``sc.Unit(unit)``.
    Whitespace between the name and unit is removed.

    Both name and unit, including brackets, are optional.
    If the unit is missing but empty brackets are present,
    ``sc.units.default_unit`` is returned.
    If the brackets are absent as well, the returned unit is ``None``.
    This ensures that columns without unit information are not accidentally assigned
    ``dimensionless`` which can silence downstream errors.

    If the name is missing, an empty string is returned.

    If the input does not conform to the expected pattern, it is returned in full
    and the unit is returned as ``None``.
    This happens, e.g., when there are multiple opening brackets (``[``).

    If the string between brackets does not represent a valid unit, the full input
    is returned as the name and the unit is returned as ``None``.

    Parameters
    ----------
    head:
        The string to parse.

    Returns
    -------
    :
        The parsed name and unit.
    """
    import re

    m = re.match(r"^([^[]*)(?:\[([^[]*)])?$", head)
    if m is None:
        return head, None

    if m.lastindex != 2:
        return m[1], None

    name = m[1].rstrip()
    if m[2].strip():
        try:
            return name, Unit(m[2])
        except UnitError:
            return head, None

    return name, default_unit


_HEADER_PARSERS = {
    "bracket": parse_bracket_header,
}


def _parse_header(header: str, parser: HeaderParserArg) -> tuple[str, Unit | None]:
    if parser is None:
        return header, default_unit
    if callable(parser):
        return parser(header)
    if (parser := _HEADER_PARSERS.get(parser)) is not None:
        return parser(header)
    else:
        raise ValueError(
            f"Unknown header parser '{parser}', "
            f"supported builtin parsers: {list(_HEADER_PARSERS.keys())}."
        )
