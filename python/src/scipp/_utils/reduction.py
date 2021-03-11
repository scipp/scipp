# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

def nonemin(a, b):
    """min which respects None.

    :param a: Operand 1.
    :param b: Operand 2.
    :returns: min(a, b) if neither arguments are None.
              Otherwise, the argument that is not None.
    :raises ValueError: If both a and b are None.
    """
    if a is None:
        if b is None:
            raise ValueError('Both arguments cannot be None in nonemin')
        return b
    if b is None:
        return a
    return min(a, b)


def nonemax(a, b):
    """max which respects None.

    :param a: Operand 1.
    :param b: Operand 2.
    :returns: max(a, b) if neither arguments are None.
              Otherwise, the argument that is not None.
    :raises ValueError: If both a and b are None.
    """
    if a is None:
        if b is None:
            raise ValueError('Both arguments cannot be None in nonemax')
        return b
    if b is None:
        return a
    return max(a, b)
