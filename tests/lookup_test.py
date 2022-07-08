# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


@pytest.mark.parametrize('mode', ['nearest', 'previous'])
def test_raises_with_histogram_if_mode_set(mode):
    da = sc.DataArray(sc.arange('x', 4), coords={'x': sc.arange('x', 5)})
    with pytest.raises(ValueError):
        sc.lookup(da, mode=mode)
