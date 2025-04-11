# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
import scipp as sc


def test_version() -> None:
    assert len(sc.__version__) > 0
