# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import Any, List

import pytest

import scipp as sc

pytest.register_assert_rewrite('scipp.testing.assertions')


def pytest_assertrepr_compare(op: str, left: Any, right: Any) -> List[str]:
    if isinstance(left, sc.Unit) and isinstance(right, sc.Unit):
        return [f'Unit({left}) {op} Unit({right})']
    if isinstance(left, sc.DType) or isinstance(right, sc.DType):
        return [f'{left!r} {op} {right!r}']
