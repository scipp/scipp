# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Dict, Optional, Union

from ..typing import DataArray, Dataset, Variable
from .argument_handlers import combine_dict_args


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
    collected_coords = combine_dict_args(coords, **coords_kwargs)

    out = self.copy(deep=False)
    for coord_key, coord in collected_coords.items():
        out.coords[coord_key] = coord

    return out


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
    collected_masks = combine_dict_args(masks, **masks_kwargs)

    out = self.copy(deep=False)
    for mask_key, mask in collected_masks.items():
        out.masks[mask_key] = mask

    return out


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
    collected_attrs = combine_dict_args(attrs, **attrs_kwargs)

    out = self.copy(deep=False)
    for attr_key, attr in collected_attrs.items():
        out.attrs[attr_key] = attr

    return out
