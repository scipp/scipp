# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Dict, Optional, Union

from ..typing import DataArray, Dataset, Variable


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
    coords_posarg = {} if coords is None else coords
    collected_coords = {**coords_posarg, **coords_kwargs}

    if len(collected_coords) != len(coords_posarg) + len(coords_kwargs):
        overlapped = set(coords).intersection(coords_kwargs)
        raise ValueError(
            'The names of coords passed in the dict '
            'and as keyword arguments must be distinct.'
            f'Following names were used in both places: {overlapped}.'
        )

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
    masks_posarg = {} if masks is None else masks
    collected_masks = {**masks_posarg, **masks_kwargs}

    if len(collected_masks) != len(masks_posarg) + len(masks_kwargs):
        overlapped = set(masks).intersection(masks_kwargs)
        raise ValueError(
            'The names of masks passed in the dict '
            'and as keyword arguments must be distinct.'
            f'Following names were used in both places: {overlapped}.'
        )

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
    attrs_posarg = {} if attrs is None else attrs
    collected_attrs = {**attrs_posarg, **attrs_kwargs}

    if len(collected_attrs) != len(attrs_posarg) + len(attrs_kwargs):
        overlapped = set(attrs_posarg).intersection(attrs_kwargs)
        raise ValueError(
            'The names of attributes passed in the dict '
            'and as keyword arguments must be distinct.'
            f'Following names were used in both places: {overlapped}.'
        )

    out = self.copy(deep=False)
    for attr_key, attr in collected_attrs.items():
        out.attrs[attr_key] = attr

    return out
