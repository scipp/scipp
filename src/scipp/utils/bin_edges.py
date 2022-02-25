# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from typing import Set

from ..core import Variable
from ..typing import VariableLike


def find_bin_edge_dims(ref: VariableLike, /, *, coord: Variable) -> Set[str]:
    """
    Returns all bin-edge dimensions of `coord` relative to the sizes of `ref`.

    Parameters
    ----------
    ref:
        Defines the reference dims and shape.
    coord:
        Defines dims to search.

    Returns
    -------
    :
        All labels of bin-edge dimensions.

    Examples
    --------

        >>> da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
        >>> coord = sc.zeros(sizes={'a': 2, 'b': 4})
        >>> sc.utils.find_bin_edge_dims(da, coord=coord)
        {'b'}

    If `ref` is a scalar, length-2 coords are assumed to be bin-edges.
    This is required to identify bin-edges in slices.

        >>> coord = sc.zeros(sizes={'a': 2})
        >>> sc.utils.find_bin_edge_dims(da['a', 0]['b', 0], coord=coord)
        {'a'}
    """
    bin_edges = set()
    for idx, dim in enumerate(coord.dims):
        length = coord.shape[idx]
        if not ref.dims:
            # Have a scalar slice.
            # Cannot match dims, just assume length 2 attributes are bin-edge
            if length == 2:
                bin_edges.add(dim)
        elif dim in ref.dims and ref.shape[ref.dims.index(dim)] + 1 == length:
            bin_edges.add(dim)
    return bin_edges


def is_bin_edges(ref: VariableLike, /, *, coord:Variable)->bool:
    """
    Returns True if `coord` has bin-edges relative to the sizes of `ref`.

    Parameters
    ----------
    ref:
        Defines the reference dims and shape.
    coord:
        Defines dims to search.

    Returns
    -------
    :
        ``True`` if any dimensions of `coord` are bin-edges compared to `ref`.

    See Also
    --------
    More details in scipp.utils.find_bin_edge_dims
    """
    return bool(find_bin_edge_dims(ref, coord=coord))
