# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock


def call_func(func, *args, out=None, **kwargs):
    if out is None:
        return func(*args, **kwargs)
    else:
        return func(*args, **kwargs, out=out)
