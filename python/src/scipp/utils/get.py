# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Gregory Tucker


def get(x, key, default=None):
    """
    Get the value associated with the provided key from a `dict`-like object

    :param x: dictlike
        Any object with `__contains__()` or `__iter__()` and `__getitem__()`
        methods which accept `key`
    :param key:
        The key to extract from the provided `dict`-like object`
    :param default:
        A default value to return if the provided key is not present
    :return:
        Either the value associated with the key or the default if the key
        is missing. If he object does not implement the required methods an
        error will be thrown.
    """
    return x[key] if key in x else default
