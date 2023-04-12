# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import Dict, Optional


def combine_dict_args(arg: Optional[Dict], /, **kwargs):
    pos_dict = {} if arg is None else arg

    overlapped = set(pos_dict).intersection(kwargs)
    if overlapped:
        raise ValueError(
            'The names passed in the dict and as keyword arguments must be distinct.'
            f'Following names are used in both arguments: {overlapped}'
        )

    return {**pos_dict, **kwargs}
