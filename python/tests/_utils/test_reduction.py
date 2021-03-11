# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import pytest
from scipp._utils.reduction import nonemin, nonemax


def test_nonemin():
    assert nonemin(1, 2) == 1
    assert nonemin(3, 2) == 2
    assert nonemin(None, 4) == 4
    assert nonemin(-1, None) == -1
    with pytest.raises(ValueError):
        nonemin(None, None)


def test_nonemax():
    assert nonemax(1, 2) == 2
    assert nonemax(3, 2) == 3
    assert nonemax(None, 4) == 4
    assert nonemax(-1, None) == -1
    with pytest.raises(ValueError):
        nonemax(None, None)
