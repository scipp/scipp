# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from collections.abc import Sequence
from typing import TypedDict


class DimsAndShape(TypedDict):
    dims: Sequence[str]
    shape: Sequence[int]


def _parse_dims_shape_sizes(
    dims: Sequence[str] | None = None,
    shape: Sequence[int] | None = None,
    sizes: dict[str, int] | None = None,
) -> DimsAndShape:
    if sizes is not None:
        if dims is not None or shape is not None:
            raise ValueError(
                "When sizes is specified, dims and shape must "
                f"both be None. Got dims: {dims}, shape: {shape}"
            )
        dims = list(sizes.keys())
        shape = list(sizes.values())
    else:
        if dims is None or shape is None:
            raise ValueError(
                "Dims and shape are required when sizes are not specified."
            )
    return {"dims": dims, "shape": shape}
