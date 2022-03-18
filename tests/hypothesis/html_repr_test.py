# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from hypothesis import given
from hypothesis import strategies as st

from scipp.testing import strategies as scst
import scipp as sc


@given(scst.variables(ndim=0))
def test_html_repr_scalar(var):
    sc.make_html(var)


@given(scst.variables(ndim=st.integers(1, 4)))
def test_html_repr_array(var):
    sc.make_html(var)


@given(scst.dataarrays())
def test_html_repr_data_array(da):
    sc.make_html(da)
    assert False
