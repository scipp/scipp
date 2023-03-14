# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import os
from pathlib import Path
from typing import Any, List

import pytest

import scipp as sc

# Load the config file in the test dir instead of the user's.
os.environ['SCIPPDIR'] = os.fspath(Path(__file__).resolve().parent)
# Silence warning from Jupyter
os.environ['JUPYTER_PLATFORM_DIRS'] = '1'

pytest.register_assert_rewrite('scipp.testing.assertions')


def pytest_assertrepr_compare(op: str, left: Any, right: Any) -> List[str]:
    if isinstance(left, sc.Unit) and isinstance(right, sc.Unit):
        return [f'Unit({left}) {op} Unit({right})']
    if isinstance(left, sc.DType) or isinstance(right, sc.DType):
        return [f'{left!r} {op} {right!r}']
