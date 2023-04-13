# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Dict, Literal, Optional, Union

from ..typing import DataArray, Dataset, Variable
from .argument_handlers import combine_dict_args


def _assign(
    obj: Union[Dataset, DataArray],
    name: Literal['coords', 'masks', 'attrs'],
    obj_attrs: Optional[Dict[str, Variable]] = None,
    /,
    **kw_obj_attrs,
) -> Union[Dataset, DataArray]:
    out = obj.copy(deep=False)
    collected = combine_dict_args(obj_attrs, kw_obj_attrs)
    for key, value in collected.items():
        getattr(out, name)[key] = value
    return out


def assign_coords(
    self, coords: Optional[Dict[str, Variable]] = None, /, **coords_kwargs
) -> Union[DataArray, Dataset]:
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
    self, masks: Optional[Dict[str, Variable]] = None, /, **masks_kwargs
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


def assign_attrs(
    self, attrs: Optional[Dict[str, Variable]] = None, /, **attrs_kwargs
) -> DataArray:
    """Return new object with updated or inserted attrs.

    Parameters
    ----------
    attrs :
        New attrs.

    attrs_kwargs :
        Keyword arguments form of ``attrs``.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated attributes.

    """
    return _assign(self, 'attrs', attrs, **attrs_kwargs)
