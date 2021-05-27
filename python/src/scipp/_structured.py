# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from . import dtype
from ._scipp.core import _element_keys, _get_elements, _set_elements


def _prop(key):
    def getter(fields):
        return _get_elements(fields._var, key)

    def setter(fields, x):
        return _set_elements(fields._var, key, x)

    return property(getter, setter)


def is_structured(obj):
    return obj.dtype in [dtype.vector_3_float64, dtype.matrix_3_float64]


def _fields(obj):
    class Fields():
        def __init__(self, var):
            self._var = var

    if is_structured(obj):
        for key in _element_keys(obj):
            setattr(Fields, key, _prop(key))
        return Fields(obj)
    return None
