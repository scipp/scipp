# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Dict, Optional, Union

from ..typing import DataArray, Dataset


def assign_coords(
    self, coords: Optional[Dict] = None, **coords_kwargs
) -> Union[DataArray, Dataset]:
    """Return new object with updated or inserted coordinate.

    Parameters
    ----------
    coords :
        New coordinates.

    **coords_kwargs :
        Keyword arguments form of ``coords``.


    Returns
    -------
    :
        ``scipp.DataArray`` or ``scipp.Dataset`` with updated coordinates.

    """

    if coords is not None:
        collected_coords = {**coords, **coords_kwargs}
    else:
        collected_coords = coords_kwargs

    for coord_key, coord in collected_coords.items():
        self.coords[coord_key] = coord

    return self


def assign_masks(self, masks: Optional[Dict] = None, **masks_kwargs) -> DataArray:
    """Update or insert masks to the ``scipp.DataArray``.

    Parameters
    ----------
    masks :
        Masks to be updated or inserted to the ``scipp.DataArray``.

    **masks_kwargs :
        Keyword arguments form of ``masks``.


    Returns
    -------
    :
        ``scipp.DataArray`` with updated masks.

    """
    if masks is not None:
        collected_masks = {**masks, **masks_kwargs}
    else:
        collected_masks = masks_kwargs

    for mask_key, mask in collected_masks.items():
        self.masks[mask_key] = mask

    return self


def assign_attrs(self, attrs: Optional[Dict] = None, **attrs_kwargs) -> DataArray:
    """Update or insert attributes to the ``scipp.DataArray``.

    Parameters
    ----------
    attrs :
        Attributes to be updated or inserted to the ``scipp.DataArray``.

    **attrs_kwargs :
        Keyword arguments form of ``attrs``.


    Returns
    -------
    :
        ``scipp.DataArray`` with updated attributes.

    """
    if attrs is not None:
        collected_attrs = {**attrs, **attrs_kwargs}
    else:
        collected_attrs = attrs_kwargs

    for attr_key, attr in collected_attrs.items():
        self.attrs[attr_key] = attr

    return self
