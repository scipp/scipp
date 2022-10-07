# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import scipp as sc
from scipp.core import concepts


def test_concrete_dims_given_single_dim_returns_dim_as_tuple():
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, 'xx') == ('xx', )


def test_concrete_dims_given_dim_list_returns_dim_tuple():
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, ['yy', 'xx']) == ('yy', 'xx')


def test_concrete_dims_given_none_returns_obj_dims():
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, None) == ('xx', 'yy')
