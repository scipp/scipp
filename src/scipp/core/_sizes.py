# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from typing import Any, Optional, Sequence, Union


def _parse_dims_shape_sizes(
    dims: Optional[Union[list[str], tuple[str, ...]]] = None,
    shape: Optional[Sequence[int]] = None,
    sizes: Optional[dict[str, int]] = None,
) -> Any:
    if sizes is not None:
        if dims is not None or shape is not None:
            raise ValueError(
                "When sizes is specified, dims and shape must "
                "both be None. Got dims: {}, shape: {}".format(dims, shape)
            )
        dims = list(sizes.keys())
        shape = list(sizes.values())
    return {"dims": dims, "shape": shape}
