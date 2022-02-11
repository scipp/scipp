# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Gregory Tucker


def get(x, key, default=None):
    """
    Get the value associated with the provided key from a `dict`-like object.

    :param x:
        Any object with a `__getitem__()` method which accept `key`.
        Otherwise, a TypeError is thrown.
    :param key:
        The key to extract from the provided `dict`-like object.
    :param default:
        A default value to return if the provided key is not present.
    :return:
        Either the value associated with the key or the default if the key
        is missing.
    """
    try:
        return x[key]
    except KeyError:
        return default
