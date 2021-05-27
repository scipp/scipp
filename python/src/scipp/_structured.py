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


class FieldsMeta(type):
    def __call__(cls, var):
        for key in _element_keys(var):
            setattr(cls, key, _prop(key))
        return type.__call__(cls, var)


class Fields(object, metaclass=FieldsMeta):
    def __init__(self, var):
        self._var = var


def is_structured(obj):
    return obj.dtype in [dtype.vector_3_float64, dtype.matrix_3_float64]


def _fields(self):
    if is_structured(self):
        return Fields(self)
    return None
