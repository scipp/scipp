# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file

import scipp as sc
from .common import assert_export


def test_reshape_variable():
    assert_export(sc.reshape, sc.Variable())


def test_reshape_data_array():
    assert_export(sc.reshape, sc.DataArray(data=sc.scalar(1.0)))


# def test_reshape_dataset():
#     assert_export(sc.reshape, sc.Dataset())
