# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import os
import time as _time


def time(method):
    """Decorator that prints function run time."""
    def timed(*args, **kw):
        indent = time.counter
        time.counter += 1
        # Not using print() since output may be swallowed in Jupyter notebooks
        os.write(1, f'{"  " * indent}enter {method.__name__}\n'.encode())
        start = _time.time()
        result = method(*args, **kw)
        end = _time.time()
        ms = (end - start) * 1000
        time.counter -= 1
        os.write(1, f'{"  " * indent}exit {method.__name__}  {ms} ms\n'.encode())
        return result

    return timed


time.counter = 0
