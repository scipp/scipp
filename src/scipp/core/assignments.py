# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Literal, TypeVar

from .argument_handlers import combine_dict_args
from .cpp_classes import DataArray, Dataset, Variable

_T = TypeVar('_T', Dataset, DataArray)


def _assign(
    obj: _T,
    name: Literal['coords', 'masks'],
    obj_attrs: dict[str, Variable] | None = None,
    /,
    **kw_obj_attrs: Variable,
) -> _T:
    out = obj.copy(deep=False)
    collected = combine_dict_args(obj_attrs, kw_obj_attrs)
    for key, value in collected.items():
        getattr(out, name)[key] = value
    return out


def assign_coords(
    self: _T, coords: dict[str, Variable] | None = None, /, **coords_kwargs: Variable
) -> _T:
    """Return new object with updated or inserted coordinate.

    Parameters
    ----------
    coords :
        New coordinates.

    coords_kwargs :
        Keyword arguments form of ``coords``.

    Returns
    -------
    :
        ``scipp.DataArray`` or ``scipp.Dataset`` with updated coordinates.

    """
    return _assign(self, 'coords', coords, **coords_kwargs)


def assign_masks(
    self: DataArray,
    masks: dict[str, Variable] | None = None,
    /,
    **masks_kwargs: Variable,
) -> DataArray:
    """Return new object with updated or inserted masks.

    Parameters
    ----------
    masks :
        New masks.

    masks_kwargs :
        Keyword arguments form of ``masks``.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated masks.

    """
    return _assign(self, 'masks', masks, **masks_kwargs)
