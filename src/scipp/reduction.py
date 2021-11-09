# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Union, List, Tuple
from .typing import VariableLike
from .core import concat, reduction


def _make_method(f):
    def method(self):
        return f(self._obj, self._dim)

    return method


class Reducer:
    all = _make_method(reduction.all)
    any = _make_method(reduction.any)
    max = _make_method(reduction.max)
    min = _make_method(reduction.min)
    sum = _make_method(reduction.sum)
    mean = _make_method(reduction.mean)
    nanmax = _make_method(reduction.nanmax)
    nanmean = _make_method(reduction.nanmean)
    nanmin = _make_method(reduction.nanmin)
    nansum = _make_method(reduction.nansum)

    def __init__(self, x: List[VariableLike]):
        self._dim = uuid.uuid4().hex
        # concat in init avoids repeated costly step in case of multiple reductions
        self._obj = concat(x, dim=self._dim)


def reduce(x: Union[List[VariableLike], Tuple[VariableLike]]) -> Reducer:
    return Reducer(list(x))
