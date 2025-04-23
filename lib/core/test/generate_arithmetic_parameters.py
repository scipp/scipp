# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# type: ignore  # noqa: PGH003
"""
This script generates input parameters to test whether arithmetic
operations are consistent with Python.

It takes the output file as its only command line argument.
"""

import sys
from itertools import product

import numpy as np


def format_number(x):
    if np.isposinf(x):
        return 'INFINITY'
    if np.isneginf(x):
        return '-INFINITY'
    if np.isnan(x):
        return f'{"-" if np.sign(x) == -1 else ""}NAN'
    return f'{x}'


def build_param(a, b):
    # implement behavior of numpy 1.20
    sign = -1 if np.sign(a) == -1 or np.sign(b) == -1 else 1
    fd = (
        sign * np.inf
        if ((isinstance(a, float) or isinstance(b, float)) and b == 0 and a != 0)
        else np.floor_divide(a, b)
    )
    return (
        f'Params{{{a}, {b}, {format_number(np.true_divide(a, b))},'
        + f' {format_number(fd)}, {format_number(np.remainder(a, b))}}}'
    )


def gen_values(dtype):
    return np.r_[np.arange(3, -4, -1), np.random.uniform(-10, 10, 5)].astype(dtype)


def main():
    np.random.seed(14653503)
    with open(sys.argv[1], 'w') as outf:
        outf.write('// SPDX-License-Identifier: BSD-3-Clause\n')
        outf.write(
            '// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)\n'
        )
        outf.write('// clang-format off\n')
        outf.write('/*\n')
        outf.write(' * This file was automatically generated\n')
        outf.write(' * DO NOT CHANGE!\n')
        outf.write(' */\n\n')

        outf.write('#include <array>\n\n')
        outf.write('#include <cmath>\n\n')

        outf.write('namespace {\n')

        name_and_dtype = (("int", int), ("float", float))
        for (a_name, a_dtype), (b_name, b_dtype) in product(
            name_and_dtype, name_and_dtype
        ):
            outf.write('template <class Params>\n')
            outf.write(
                'constexpr inline auto '
                f'division_params_{a_name}_{b_name} = std::array{{\n'
            )
            for a, b in product(gen_values(a_dtype), gen_values(b_dtype)):
                outf.write(build_param(a, b) + ',\n')
            outf.write('};\n')

        outf.write('} // namespace\n')
        outf.write('// clang-format on\n')


if __name__ == "__main__":
    main()
