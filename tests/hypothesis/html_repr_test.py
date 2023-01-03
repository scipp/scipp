# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from hypothesis import given, settings
from hypothesis import strategies as st

import scipp as sc
from scipp.testing import strategies as scst

# Tests have an increased deadline because the default was not enough for our MacOS CI.
# hypothesis failed the tests because of it but subsequent invocations during
# minimization of the failing example could not reproduce the time-based failure,
# making hypothesis flag the test as flaky.
#
# This might indicate some hidden start up cost.
# It might be related to complicated utf-8 strings as all failing cases contain many
# unusual characters, many of which cannot be rendered properly on GitHub.


@settings(deadline=1000)
@given(scst.variables(ndim=0))
def test_html_repr_scalar(var):
    sc.make_html(var)


@settings(deadline=1000)
@given(scst.variables(ndim=st.integers(1, 4)))
def test_html_repr_array(var):
    sc.make_html(var)


@settings(deadline=1000)
@given(scst.dataarrays())
def test_html_repr_data_array(da):
    sc.make_html(da)
