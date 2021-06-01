# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Gregory Tucker

def get(x, key, default=None):
    """
    Get the value associated with the provided key from a `dict`-like object

    :param x: dictlike
        Any object with an `items()` *or* both `keys()` and `values()` method(s)
    :param key:
        The key to extract from the provided `dict`-like object`
    :param default:
        A default value to return if the provided key is not present
    :return:
        Either the value associated with the key or the default if the key is missing
        or the object does not implement the required methods.
    """

    return_value = default
    if hasattr(x, 'items') and callable(x.items):
        for k, v in x.items():
            if k == key:
                return_value = v
    elif hasattr(x, 'keys') and callable(x.keys) and hasattr(x, 'values') and callable(x.values):
        for k, v in zip(x.keys(), x.values()):
            if k == key:
                return_value = v
    return return_value

