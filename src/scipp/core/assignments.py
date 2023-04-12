# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Dict, Union

from ..typing import DataArray, Dataset


def assign_coords(self, coords: Dict) -> Union[DataArray, Dataset]:
    """Return new object with updated or inserted coordinate.

    Parameters
    ----------
    coords :
        New coordinates.

    Returns
    -------
    :
        ``scipp.DataArray`` or ``scipp.Dataset`` with updated coordinates.

    """

    out = self.copy(deep=False)
    for coord_key, coord in coords.items():
        out.coords[coord_key] = coord

    return out


def assign_masks(self, masks: Dict) -> DataArray:
    """Return new object with updated or inserted masks.

    Parameters
    ----------
    masks :
        New masks.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated masks.

    """
    out = self.copy(deep=False)
    for mask_key, mask in masks.items():
        out.masks[mask_key] = mask

    return out


def assign_attrs(self, attrs: Dict) -> DataArray:
    """Return new object with updated or inserted attrs.

    Parameters
    ----------
    attrs :
        New attrs.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated attributes.

    """
    out = self.copy(deep=False)
    for attr_key, attr in attrs.items():
        out.attrs[attr_key] = attr

    return out
