# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import os
from typing import Any

import hypothesis
import pytest

import scipp as sc

# Silence warning from Jupyter
os.environ['JUPYTER_PLATFORM_DIRS'] = '1'

pytest.register_assert_rewrite('scipp.testing.assertions')


def pytest_assertrepr_compare(op: str, left: Any, right: Any) -> list[str] | None:
    if isinstance(left, sc.Unit) and isinstance(right, sc.Unit):
        return [f'Unit({left}) {op} Unit({right})']
    if isinstance(left, sc.DType) or isinstance(right, sc.DType):
        return [f'{left!r} {op} {right!r}']
    return None


hypothesis.settings.register_profile(
    "ci",
    # Disable the time limit, see
    # https://github.com/scipp/scipp/issues/3721
    # https://github.com/astropy/astropy/issues/17649
    deadline=None,
    # Print the random seed blob for reproducibility:
    print_blob=True,
)
