# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp.core import _element_keys, _get_elements, _set_elements, dtype


def _prop(key):
    def getter(fields):
        return _get_elements(fields._var, key)

    def setter(fields, x):
        return _set_elements(fields._var, key, x)

    return property(getter, setter)


def is_structured(obj):
    if obj.bins is not None:
        return is_structured(obj.bins.constituents['data'])
    return obj.dtype in [dtype.vector_3_float64, dtype.matrix_3_float64]


def _fields(obj):
    class Fields():
        def __init__(self, keys, var):
            self._var = var
            self._keys = keys

        def __contains__(self, key):
            return key in self._keys

        def __iter__(self):
            return self._keys.__iter__()

        def __getitem__(self, key):
            return _get_elements(self._var, key)

        def __setitem__(self, key, x):
            return _set_elements(self._var, key, x)

        def keys(self):
            yield from self._keys

        def values(self):
            yield from (self[key] for key in self)

        def items(self):
            yield from ((key, self[key]) for key in self)

    if is_structured(obj):
        keys = _element_keys(obj)
        for key in keys:
            setattr(Fields, key, _prop(key))
        return Fields(keys=keys, var=obj)
    return None
