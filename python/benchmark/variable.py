# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
import time
import scipp as sc
from scipp import Dim
import numpy as np


def assign_from_numpy_1d(size):
    a = np.arange(size, dtype=np.float64)

    v = sc.Variable([Dim.X], a)

    n_iterations = 10
    total_time = 0

    for _ in range(n_iterations):
        start_time = time.perf_counter()
        v.values = a
        end_time = time.perf_counter()
        total_time += end_time - start_time

    size_bytes = size * np.dtype(np.float64).itemsize

    read_write_factor = 3
    bytes_per_second = (n_iterations * read_write_factor * size_bytes) \
        / total_time

    total_time /= n_iterations
    total_time *= 1e3

    return {
        'n_elements': size,
        'mbytes': size_bytes / 1e6,
        'avg_iteration_time': total_time,
        'mbytes_per_second': bytes_per_second / 1e6,
    }


if __name__ == '__main__':
    for a in [1e7, 1e8, 1e9, 5e9]:
        print('assign_from_numpy_1d', assign_from_numpy_1d(int(a)))
