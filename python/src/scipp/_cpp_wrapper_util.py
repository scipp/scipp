# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock


def call_func(func, *args, out=None, **kwargs):
    if out is None:
        return func(*args, **kwargs)
    else:
        return func(*args, **kwargs, out=out)
