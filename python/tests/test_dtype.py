# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import numpy as np
import scipp as sc


def test_dtype():
    assert sc.dtype.int32 == sc.dtype.int32
    assert sc.dtype.int32 != sc.dtype.int64


@pytest.mark.skip(reason="Unfortunately the scipp dtype is currently not \
        compatible with the numpy dtype. Scippy supports types such as \
        strings which numpy cannot handle, so we cannot simply use \
        numpy.dtype.")
def test_numpy_comparison():
    assert sc.dtype.int32 == np.dtype(np.int32)
