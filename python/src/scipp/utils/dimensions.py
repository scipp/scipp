# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen


def find_bin_edges(var, ds):
    """
    Returns a list of bin-edge dimension labels assuming that
    var is a coord or attr of ds.
    """
    bin_edges = []
    for idx, dim in enumerate(var.dims):
        length = var.shape[idx]
        if not ds.dims:
            # Have a scalar slice.
            # Cannot match dims, just assume length 2 attributes are bin-edge
            if length == 2:
                bin_edges.append(dim)
        elif dim in ds.dims and ds.shape[ds.dims.index(dim)] + 1 == length:
            bin_edges.append(dim)
    return bin_edges
