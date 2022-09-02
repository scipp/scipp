# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import numpy as np
from scipp import scalar
from ...factory import make_dense_data_array, make_dense_dataset


def _get_child(plot, n=0):
    children = plot._children
    keys = list(children.keys())
    return children[keys[n]]


def _get_line(plot, n=0):
    return _get_child(plot=plot, n=n)._line


def _get_mesh(plot):
    return _get_child(plot)._mesh


def test_kwarg_linecolor():
    da = make_dense_data_array(ndim=1)
    p = da.plot(color='r')
    _ = p.to_widget()
    assert _get_line(p).get_color() == 'r'
    p = da.plot(c='b')
    _ = p.to_widget()
    assert _get_line(p).get_color() == 'b'


def test_kwarg_linestyle():
    da = make_dense_data_array(ndim=1)
    p = da.plot(linestyle='solid')
    _ = p.to_widget()
    assert _get_line(p).get_linestyle() == '-'
    p = da.plot(ls='dashed')
    _ = p.to_widget()
    assert _get_line(p).get_linestyle() == '--'


def test_kwarg_linewidth():
    da = make_dense_data_array(ndim=1)
    p = da.plot(linewidth=3)
    _ = p.to_widget()
    assert _get_line(p).get_linewidth() == 3
    p = da.plot(lw=5)
    _ = p.to_widget()
    assert _get_line(p).get_linewidth() == 5


def test_kwarg_marker():
    da = make_dense_data_array(ndim=1)
    p = da.plot(marker='+')
    _ = p.to_widget()
    assert _get_line(p).get_marker() == '+'


def test_kwarg_norm():
    da = make_dense_data_array(ndim=1)
    p = da.plot(norm='log')
    _ = p.to_widget()
    assert p._ax.get_yscale() == 'log'


def test_kwarg_scale():
    da = make_dense_data_array(ndim=1)
    p = da.plot(scale={'xx': 'log'})
    _ = p.to_widget()
    assert p._ax.get_xscale() == 'log'
    assert p._ax.get_yscale() == 'linear'


def test_kwarg_cmap():
    da = make_dense_data_array(ndim=2)
    p = da.plot(cmap='magma')
    _ = p.to_widget()
    assert _get_mesh(p).get_cmap().name == 'magma'


def test_kwarg_scale_2d():
    da = make_dense_data_array(ndim=2)
    p = da.plot(scale={'xx': 'log', 'yy': 'log'})
    _ = p.to_widget()
    assert p._ax.get_xscale() == 'log'
    assert p._ax.get_yscale() == 'log'


def test_kwarg_crop_1d_min_max():
    da = make_dense_data_array(ndim=1)
    p = da.plot(crop={'xx': {'min': scalar(20, unit='m'), 'max': scalar(40, unit='m')}})
    _ = p.to_widget()
    assert np.array_equal(p._ax.get_xlim(), [20, 40])


def test_kwarg_crop_1d_min_only():
    da = make_dense_data_array(ndim=1)
    p = da.plot(crop={'xx': {'min': scalar(20, unit='m')}})
    _ = p.to_widget()
    assert p._ax.get_xlim()[0] == 20


def test_kwarg_crop_1d_min_conversion():
    da = make_dense_data_array(ndim=1)
    p = da.plot(crop={'xx': {'min': scalar(200, unit='cm')}})
    _ = p.to_widget()
    assert p._ax.get_xlim()[0] == 2


def test_kwarg_crop_2d():
    da = make_dense_data_array(ndim=2)
    p = da.plot(
        crop={
            'xx': {
                'min': scalar(20, unit='m')
            },
            'yy': {
                'min': scalar(10, unit='m'),
                'max': scalar(4000, unit='cm')
            }
        })
    _ = p.to_widget()
    assert p._ax.get_xlim()[0] == 20
    assert np.array_equal(p._ax.get_ylim(), [10, 40])


def test_kwarg_for_two_lines():
    ds = make_dense_dataset(ndim=1)
    p = ds.plot(color='r')
    _ = p.to_widget()
    assert _get_line(p, 0).get_color() == 'r'
    assert _get_line(p, 1).get_color() == 'r'


def test_kwarg_as_dict():
    ds = make_dense_dataset(ndim=1)
    p = ds.plot(color={'a': 'red', 'b': 'black'})
    _ = p.to_widget()
    assert _get_line(p, 0).get_color() == 'red'
    assert _get_line(p, 1).get_color() == 'black'
