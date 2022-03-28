# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
from typing import Dict, List, Optional, Sequence, Tuple, Union


def _parse_dims_shape_sizes(dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
                            shape: Optional[Sequence[int]] = None,
                            sizes: Optional[Dict[str, int]] = None):
    if sizes is not None:
        if dims is not None or shape is not None:
            raise ValueError("When sizes is specified, dims and shape must "
                             "both be None. Got dims: {}, shape: {}".format(
                                 dims, shape))
        dims = list(sizes.keys())
        shape = list(sizes.values())
    return {"dims": dims, "shape": shape}
