# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from collections.abc import Iterable

import pytest
import scipy.constants as theirs

import scipp as sc
import scipp.constants as ours


def constant_names() -> Iterable[str]:
    return filter(
        lambda name: not name.startswith('_')
        and isinstance(getattr(ours, name), sc.Variable),
        dir(ours),
    )


@pytest.mark.parametrize("name", constant_names())
def test_constant(name) -> None:
    assert getattr(ours, name).value == getattr(theirs, name)


def test_physical_constants() -> None:
    v, u, e = theirs.physical_constants['speed of light in vacuum']
    var = ours.physical_constants('speed of light in vacuum')
    assert var.value == v
    assert var.unit == u
    assert var.variance is None
    var = ours.physical_constants('speed of light in vacuum', with_variance=True)
    assert var.variance == e * e


@pytest.mark.parametrize(
    "var", [ours.m_e, ours.physical_constants('speed of light in vacuum')]
)
def test_constants_cannot_be_modified(var) -> None:
    original = var.copy()
    with pytest.raises(sc.VariableError):
        var *= 2.0
    assert sc.identical(var, original)
