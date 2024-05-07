# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet


def name_with_unit(var=None, name=None, log=False):
    """
    Make a column title or axis label with "Name [unit]".
    """
    text = ""
    if name is not None:
        text = name
    elif var is not None:
        text = str(var.dims[-1])

    if log:
        text = "log\u2081\u2080(" + text + ")"
    if var is not None:
        unit = var.unit if var.bins is None else var.bins.constituents["data"].unit
        if unit is not None:
            text += f" [{unit}]"
    return text


def value_to_string(val, precision=3, max_str_len=80):
    """
    Convert a number to a human readable string.
    """
    if (not isinstance(val, float)) or (val == 0):
        raw_text = str(val)
        if len(raw_text) > max_str_len:
            text = f"{raw_text[:max_str_len-3]}..."
        else:
            text = raw_text
    elif (abs(val) >= 10.0 ** (precision + 1)) or (
        abs(val) <= 10.0 ** (-precision - 1)
    ):
        text = "{val:.{prec}e}".format(val=val, prec=precision)
    else:
        text = f"{val}"
        if len(text) > precision + 2 + (text[0] == '-'):
            text = "{val:.{prec}f}".format(val=val, prec=precision)
    return text
