# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import os
import time as _time
from collections.abc import Callable
from typing import ParamSpec, TypeVar

_P = ParamSpec('_P')
_R = TypeVar('_R')


def time(method: Callable[_P, _R]) -> Callable[_P, _R]:
    """Decorator that prints function run time."""

    def timed(*args: _P.args, **kw: _P.kwargs) -> _R:
        indent = time.counter  # type: ignore[attr-defined]
        time.counter += 1  # type: ignore[attr-defined]
        # Not using print() since output may be swallowed in Jupyter notebooks
        os.write(1, f'{"  " * indent}enter {method.__name__}\n'.encode())
        start = _time.time()
        result = method(*args, **kw)
        end = _time.time()
        ms = (end - start) * 1000
        time.counter -= 1  # type: ignore[attr-defined]
        os.write(1, f'{"  " * indent}exit {method.__name__}  {ms} ms\n'.encode())
        return result

    return timed


time.counter = 0  # type: ignore[attr-defined]
