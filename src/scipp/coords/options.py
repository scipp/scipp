# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses


@dataclasses.dataclass(frozen=True)
class Options:
    """
    Optional arguments of transform_coords.
    """

    rename_dims: bool
    keep_aliases: bool
    keep_intermediate: bool
    keep_inputs: bool
    quiet: bool
