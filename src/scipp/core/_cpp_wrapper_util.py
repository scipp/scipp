# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import itertools

from .data_group import DataGroup, data_group_nary


def call_func(func, *args, out=None, **kwargs):
    if any([isinstance(x, DataGroup) for x in itertools.chain(args, kwargs.values())]):
        if out is not None:
            raise ValueError(
                "`out` argument is not supported for DataGroup operations."
            )
        return data_group_nary(func, *args, **kwargs)
    if out is None:
        return func(*args, **kwargs)
    else:
        return func(*args, **kwargs, out=out)
