# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_to_buckets_default_begin_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    var = sc.to_buckets(dim='x', data=data)
    assert var.dims == data.dims
    assert var.shape == data.shape
    for i in range(4):
        assert sc.is_equal(var['x', i].value, data['x', i])


def test_to_buckets_default_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    var = sc.to_buckets(begin=begin, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.is_equal(var['y', 0].value, data['x', 1])
    assert sc.is_equal(var['y', 1].value, data['x', 3])


def test_to_buckets_fail_only_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    end = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    with pytest.raises(RuntimeError):
        sc.to_buckets(end=end, dim='x', data=data)


def test_to_buckets():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.dtype.int64)
    var = sc.to_buckets(begin=begin, end=end, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.is_equal(var['y', 0].value, data['x', 0:2])
    assert sc.is_equal(var['y', 1].value, data['x', 2:4])
