# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
import scipp as sc


def test_version():
    assert len(sc.__version__) > 0
