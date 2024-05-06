# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import itertools
from typing import TYPE_CHECKING, Callable, ParamSpec, TypeVar, Union

from .data_group import DataGroup, data_group_nary

if TYPE_CHECKING:
    from .data_group import DataGroup

_R = TypeVar('_R')
_P = ParamSpec('_P')


def call_func(
    func: Callable[_P, _R],
    *args: _P.args,
    **kwargs: _P.kwargs,
) -> Union[_R, DataGroup]:
    out = kwargs.pop('out', None)
    if any(isinstance(x, DataGroup) for x in itertools.chain(args, kwargs.values())):
        if out is not None:
            raise ValueError(
                "`out` argument is not supported for DataGroup operations."
            )
        return data_group_nary(func, *args, **kwargs)
    if out is None:
        return func(*args, **kwargs)
    else:
        return func(*args, **kwargs, out=out)  # type: ignore[arg-type]
