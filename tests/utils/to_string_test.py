# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import scipp.utils as su


def test_int_to_string() -> None:
    assert su.value_to_string(3) == '3'


def test_float_to_string() -> None:
    assert su.value_to_string(12.34) == '12.34'


def test_long_float_to_string() -> None:
    assert su.value_to_string(12.3456789) == '12.346'


def test_long_float_to_string_high_prec() -> None:
    assert su.value_to_string(12.3456789, precision=5) == '12.34568'


def test_large_float_to_string() -> None:
    assert su.value_to_string(123456.7890) == '1.235e+05'


def test_small_float_to_string() -> None:
    assert su.value_to_string(0.0000000123456) == '1.235e-08'


def test_negative_float_to_string() -> None:
    assert su.value_to_string(-12.345678) == '-12.346'


def test_string_to_string() -> None:
    assert su.value_to_string('abc') == 'abc'
