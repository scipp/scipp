# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
from typing import Any


def value_to_string(val: Any, precision: int = 3, max_str_len: int = 80) -> str:
    """
    Convert a number to a human-readable string.
    """
    if (not isinstance(val, float)) or (val == 0):
        raw_text = str(val)
        if len(raw_text) > max_str_len:
            text = f"{raw_text[: max_str_len - 3]}..."
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
