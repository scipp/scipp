# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
import scipp._utils as su


def test_label_var_with_unit():
    var = sc.Variable(['x'], values=np.random.random(10), unit=sc.units.m)
    assert su.name_with_unit(var) == 'x [m]'


def test_label_var_with_unit_with_log():
    var = sc.Variable(['x'], values=np.random.random(10), unit=sc.units.m)
    assert su.name_with_unit(var, log=True) == 'log\u2081\u2080(x) [m]'


def test_label_var_no_unit():
    var = sc.Variable(['x'], values=np.random.random(10))
    assert su.name_with_unit(var) == 'x [dimensionless]'


def test_label_name_only():
    assert su.name_with_unit(name='My name') == 'My name'


def test_label_name_with_unit():
    var = sc.Variable(['x'], values=np.random.random(10), unit=sc.units.counts)
    assert su.name_with_unit(var, name='sample') == 'sample [counts]'


def test_int_to_string():
    assert su.value_to_string(3) == '3'


def test_float_to_string():
    assert su.value_to_string(12.34) == '12.34'


def test_long_float_to_string():
    assert su.value_to_string(12.3456789) == '12.346'


def test_long_float_to_string_high_prec():
    assert su.value_to_string(12.3456789, precision=5) == '12.34568'


def test_large_float_to_string():
    assert su.value_to_string(123456.7890) == '1.235e+05'


def test_small_float_to_string():
    assert su.value_to_string(0.0000000123456) == '1.235e-08'


def test_negative_float_to_string():
    assert su.value_to_string(-12.345678) == '-12.346'


def test_string_to_string():
    assert su.value_to_string('abc') == 'abc'
