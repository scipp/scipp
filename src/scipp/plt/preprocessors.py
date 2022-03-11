# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .slider_widget import SliderWidget
from .mask_widget import MaskWidget


class Preprocessor:
    def __init__(self, func, widget):
        self.func = func
        self.widget = widget


def _slice_data(data_array, slices):
    """
    Slice the data along dimension sliders that are not disabled for all
    entries in the dict of data arrays, and return a dict of 1d value
    arrays for data values, variances, and masks.
    """
    out = data_array
    for dim, sl in slices.items():
        out = out[dim, sl]
    return out


def _hide_masks(data_array, masks):
    out = data_array.copy()
    for name, value in masks:
        if not value:
            del out.masks[name]
    return out


def make_default_preprocessors(data_array, ndim):

    slicing_preprocessor = Preprocessor(
        func=_slice_data,
        widget=SliderWidget(
            dims=data_array.dims,
            # formatters=formatters,
            ndim=ndim,
            # dim_label_map=labels,
            # masks=self._scipp_obj_dict,
            sizes=data_array.sizes))

    mask_preprocessor = Preprocessor(func=_hide_masks,
                                     widget=MaskWidget(data_array.masks))

    return [slicing_preprocessor, mask_preprocessor]
