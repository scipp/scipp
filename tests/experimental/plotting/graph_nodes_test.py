# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
from scipp.experimental.plotting import Plot, Figure, widgets, input_node, node
from scipp.experimental.plotting.widgets import widget_node
from ...factory import make_dense_data_array, make_dense_dataset
import ipywidgets as ipw


@node
def hide_masks(data_array, masks):
    out = data_array.copy(deep=False)
    for name, value in masks.items():
        if name in out.masks and (not value):
            del out.masks[name]
    return out


def test_single_1d_line():
    da = make_dense_data_array(ndim=1)
    n = input_node(da)
    fig = Figure(n)
    fig.render()


def test_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    a = input_node(ds['a'])
    b = input_node(ds['b'])
    fig = Figure(a, b)
    fig.render()


def test_difference_of_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    a = input_node(ds['a'])
    b = input_node(ds['b'])

    @node
    def diff(x, y):
        return x - y

    c = diff(a, b)
    fig = Figure(a, b, c)
    fig.render()


def test_2d_image():
    da = make_dense_data_array(ndim=2)
    a = input_node(da)
    fig = Figure(a)
    fig.render()


def test_2d_image_smoothing_slider():
    da = make_dense_data_array(ndim=2)
    a = input_node(da)

    sl = ipw.IntSlider(min=1, max=10)
    sigma_node = widget_node(sl)

    from scipp.ndimage import gaussian_filter
    smooth_node = node(gaussian_filter)(a, sigma=sigma_node)

    fig = Figure(smooth_node)
    Plot([fig, sl])
    fig.render()
    sl.value = 5


def test_2d_image_with_masks():
    da = make_dense_data_array(ndim=2)
    da.masks['m1'] = da.data < sc.scalar(0.0, unit='counts')
    da.masks['m2'] = da.coords['xx'] > sc.scalar(30., unit='m')

    a = input_node(da)

    widget = widgets.Checkboxes(da.masks.keys())
    w = widget_node(widget)

    masks_node = hide_masks(a, w)
    fig = Figure(masks_node)
    Plot([fig, widget])
    fig.render()
    widget.toggle_all_button.value = False


def test_two_1d_lines_with_masks():
    ds = make_dense_dataset()
    ds['a'].masks['m1'] = ds['a'].coords['xx'] > sc.scalar(40.0, unit='m')
    ds['a'].masks['m2'] = ds['a'].data < ds['b'].data
    ds['b'].masks['m1'] = ds['b'].coords['xx'] < sc.scalar(5.0, unit='m')

    a = input_node(ds['a'])
    b = input_node(ds['b'])

    widget = widgets.Checkboxes(list(ds['a'].masks.keys()) + list(ds['b'].masks.keys()))
    w = widget_node(widget)

    node_masks_a = hide_masks(a, w)
    node_masks_b = hide_masks(b, w)
    fig = Figure(node_masks_a, node_masks_b)
    Plot([fig, widget])
    fig.render()
    widget.toggle_all_button.value = False


def test_node_sum_data_along_y():
    da = make_dense_data_array(ndim=2, binedges=True)
    a = input_node(da)

    s = node(sc.sum, dim='yy')(a)

    fig1 = Figure(a)
    fig2 = Figure(s)
    Plot([[fig1, fig2]])
    fig1.render()
    fig2.render()


def test_slice_3d_cube():
    da = make_dense_data_array(ndim=3)
    a = input_node(da)
    sl = widgets.SliceWidget(da, ['zz'])
    w = widget_node(sl)

    slice_node = widgets.slice_dims(a, w)
    sl.make_view(slice_node)

    fig = Figure(slice_node)
    Plot([fig, sl])
    fig.render()
    sl.controls["zz"]["slider"].value = 10


def test_3d_image_slicer_with_connected_side_histograms():
    da = make_dense_data_array(ndim=3)
    a = input_node(da)
    sl = widgets.SliceWidget(da, ['zz'])
    w = widget_node(sl)

    sliced = widgets.slice_dims(a, w)
    sl.make_view(sliced)
    fig = Figure(sliced)

    histx = node(sc.sum, dim='xx')(sliced)
    histy = node(sc.sum, dim='yy')(sliced)

    fx = Figure(histx)
    fy = Figure(histy)
    Plot([[fx, fy], fig, sl])
    fig.render()
    fx.render()
    fy.render()
    sl.controls["zz"]["slider"].value = 10
