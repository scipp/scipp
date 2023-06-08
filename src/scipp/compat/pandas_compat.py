# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import TYPE_CHECKING, Callable, Iterable, Literal, Optional, Tuple, Union

from ..core import DataArray, Dataset, Unit, array
from ..typing import VariableLike
from ..units import default_unit

if TYPE_CHECKING:
    import pandas as pd


def from_pandas_series(
    se: pd.Series, *, include_index: bool = True, head_parser: HeadParserArg = None
) -> DataArray:
    row_index = se.axes[0]
    row_index_name = "row" if row_index.name is None else str(row_index.name)
    name, unit = (
        ("", default_unit)
        if se.name is None
        else _parse_head(str(se.name), head_parser)
    )

    coords = (
        {row_index_name: array(dims=[row_index_name], values=row_index)}
        if include_index
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
    data_columns: Optional[Union[str, Iterable[str]]] = None,
    include_index: bool = True,
    head_parser: HeadParserArg = None,
) -> Dataset:
    import pandas as pd

    columns = (
        from_pandas_series(
            pd.Series(df[column_name]),
            include_index=include_index,
            head_parser=head_parser,
        )
        for column_name in df.axes[1]
    )
    coords = {da.name: da for da in columns}

    if data_columns is None:
        data = coords
        coords = {}
    else:
        if isinstance(data_columns, str):
            data_columns = (data_columns,)
        data = {name: coords.pop(name) for name in data_columns}
        coords = {name: coord.data for name, coord in coords.items()}

    return Dataset(data, coords=coords)


def from_pandas(
    pd_obj: Union[pd.DataFrame, pd.Series],
    *,
    data_columns: Optional[Union[str, Iterable[str]]] = None,
    include_index: bool = True,
    head_parser: HeadParserArg = None,
) -> VariableLike:
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
    include_index:
        If True, the row index is included in the output as a coordinate.
    head_parser:
        Parses each column name to extract a name and unit for each data array.
        By default, it returns the column name and uses the default unit.
        Builtin parsers can be specified by name:

        - ``"bracket"``: Parses strings of the form ``name [unit]`` where ``name``
          be any string that does not contain the character ``[``.
          Whitespace between the name and unit is removed.
          Both name and unit, including brackets, are optional.

        When implementing a parser, make sure to return ``sc.units.default_unit``
        if the unit is not specified instead of ``sc.Unit("")`` to get a sensible
        result for non-numeric columns.

    Returns
    -------
    :
        The converted scipp object.
    """
    import pandas as pd

    if isinstance(pd_obj, pd.DataFrame):
        return from_pandas_dataframe(
            pd_obj,
            data_columns=data_columns,
            include_index=include_index,
            head_parser=head_parser,
        )
    elif isinstance(pd_obj, pd.Series):
        return from_pandas_series(
            pd_obj, include_index=include_index, head_parser=head_parser
        )
    else:
        raise ValueError(f"from_pandas: cannot convert type '{type(pd_obj)}'")


HeadParser = Callable[[str], Tuple[str, Unit]]
HeadParserArg = Optional[Union[Literal["bracket"], HeadParser]]


def parse_bracket_head(head: str) -> Tuple[str, Unit]:
    import re

    m = re.match(r"^([^[]*)(?:\[([^[]*)])?$", head)
    if m is None:
        raise ValueError(
            f"Column header does not conform to pattern `name [unit]`: {head}"
        )

    if m.lastindex == 2:
        name = m[1].rstrip()
        unit = Unit(m[2]) if m[2] else default_unit
    else:
        name = m[1]
        unit = default_unit
    return name, unit


_HEAD_PARSERS = {
    "bracket": parse_bracket_head,
}


def _parse_head(head: str, parser: HeadParserArg) -> Tuple[str, Unit]:
    if parser is None:
        return head, default_unit
    if callable(parser):
        return parser(head)
    if (parser := _HEAD_PARSERS.get(parser)) is not None:
        return parser(head)
    else:
        raise ValueError(
            f"Unknown head parser '{parser}', "
            f"supported builtin parsers: {list(_HEAD_PARSERS.keys())}."
        )
