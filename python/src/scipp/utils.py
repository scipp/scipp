# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from ._scipp.core.units import dimensionless
from ._scipp.core import (Variable, VariableProxy, Dataset, DatasetProxy,
                          DataArray, DataProxy)


def name_with_unit(var=None, name=None, log=False, replace_dim=True):
    """
    Make a column title or axis label with "Name [unit]".
    """
    text = ""
    if name is not None:
        text = name
    elif var is not None:
        text = str(var.dims[-1])
        if replace_dim:
            text = text.replace("Dim.", "")

    if log:
        text = "log\u2081\u2080(" + text + ")"
    if var is not None:
        if var.unit != dimensionless:
            text += " [{}]".format(var.unit)
    return text


def value_to_string(val, precision=3):
    """
    Convert a number to a human readable string.
    """
    if (not isinstance(val, float)) or (val == 0):
        text = str(val)
    elif (abs(val) >= 10.0**(precision+1)) or \
         (abs(val) <= 10.0**(-precision-1)):
        text = "{val:.{prec}e}".format(val=val, prec=precision)
    else:
        text = "{}".format(val)
        if len(text) > precision + 2 + (text[0] == '-'):
            text = "{val:.{prec}f}".format(val=val, prec=precision)
    return text


def is_variable(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, Variable) or isinstance(obj, VariableProxy)


def is_dataset(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, Dataset) or isinstance(obj, DatasetProxy)


def is_data_array(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, DataArray) or isinstance(obj, DataProxy)


def is_dataset_or_array(obj):
    """
    Return True if the input object is either a Dataset or DataArray.
    """
    return is_dataset(obj) or is_data_array(obj)
