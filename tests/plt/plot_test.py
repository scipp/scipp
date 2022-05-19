# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
from scipp.plt import Plot, Figure, widgets, Node, node
from ..factory import make_dense_data_array, make_dense_dataset
import matplotlib
import ipywidgets as ipw

matplotlib.use('Agg')


def test_plot_single_1d_line():
    da = make_dense_data_array(ndim=1)
    n = Node(da)
    fig = Figure(n)
    fig.render()


def test_plot_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    a = Node(ds['a'])
    b = Node(ds['b'])
    fig = Figure(a, b)
    fig.render()


def test_plot_difference_of_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    a = Node(ds['a'])
    b = Node(ds['b'])

    @node
    def diff(x, y):
        return x - y

    c = diff(a, b)
    fig = Figure(a, b, c)
    fig.render()


def test_plot_2d_image():
    da = make_dense_data_array(ndim=2)
    a = Node(da)
    fig = Figure(a)
    fig.render()


def test_plot_2d_image_smoothing_slider():
    da = make_dense_data_array(ndim=2)
    a = Node(da)

    sl = ipw.IntSlider(min=1, max=10)
    sigma_node = Node(lambda: sl.value)
    sl.observe(sigma_node.notify_children, names="value")

    from scipy.ndimage import gaussian_filter

    @node
    def smooth(da, sigma):
        out = da.copy()
        out.values = gaussian_filter(da.values, sigma=sigma)
        return out

    smooth_node = smooth(a, sigma=sigma_node)
    fig = Figure(smooth_node)
    Plot([fig, sl])
    fig.render()
    sl.value = 5


def test_plot_2d_image_with_masks():
    da = make_dense_data_array(ndim=2)
    da.masks['m1'] = da.data < sc.scalar(0.0, unit='counts')
    da.masks['m2'] = da.coords['xx'] > sc.scalar(30., unit='m')

    a = Node(da)

    widget = widgets.MaskWidget(da.masks)
    w = Node(lambda: widget.value)
    widget.observe(w.notify_children, names="value")

    masks_node = widgets.hide_masks(a, w)
    fig = Figure(masks_node)
    Plot([fig, widget])
    fig.render()
    widget._all_masks_button.value = False


def test_plot_two_1d_lines_with_masks():
    ds = make_dense_dataset()
    ds['a'].masks['m1'] = ds['a'].coords['xx'] > sc.scalar(40.0, unit='m')
    ds['a'].masks['m2'] = ds['a'].data < ds['b'].data
    ds['b'].masks['m1'] = ds['b'].coords['xx'] < sc.scalar(5.0, unit='m')

    a = Node(ds['a'])
    b = Node(ds['b'])

    widget_a = widgets.MaskWidget(ds['a'].masks)
    widget_b = widgets.MaskWidget(ds['b'].masks)
    w_a = Node(lambda: widget_a.value)
    w_b = Node(lambda: widget_b.value)
    widget_a.observe(w_a.notify_children, names="value")
    widget_b.observe(w_b.notify_children, names="value")

    node_masks_a = widgets.hide_masks(a, w_a)
    node_masks_b = widgets.hide_masks(b, w_b)
    fig = Figure(node_masks_a, node_masks_b)
    Plot([fig, [widget_a, widget_b]])
    fig.render()
    widget_a._all_masks_button.value = False
    widget_b._all_masks_button.value = False


def test_plot_node_sum_data_along_y():
    da = make_dense_data_array(ndim=2, binedges=True)
    a = Node(da)

    @node
    def sumy(da):
        return da.sum('yy')

    s = sumy(a)
    fig1 = Figure(a)
    fig2 = Figure(s)
    Plot([[fig1, fig2]])
    fig1.render()
    fig2.render()


def test_plot_slice_3d_cube():
    da = make_dense_data_array(ndim=3)
    a = Node(da)
    sl = widgets.SliceWidget(da, ['zz'])
    input_node = Node(lambda: sl.value)
    sl.observe(input_node.notify_children, names="value")

    slice_node = widgets.slice_dims(a, input_node)

    fig = Figure(slice_node)
    Plot([fig, sl])
    fig.render()
    sl._controls["zz"]["slider"].value = 10


def test_plot_3d_image_slicer_with_connected_side_histograms():
    da = make_dense_data_array(ndim=3)
    a = Node(da)
    sl = widgets.SliceWidget(da, ['zz'])
    input_node = Node(lambda: sl.value)
    sl.observe(input_node.notify_children, names="value")

    sliced = widgets.slice_dims(a, input_node)
    fig = Figure(sliced)

    @node
    def sumx(da):
        return da.sum('xx')

    @node
    def sumy(da):
        return da.sum('yy')

    histx = sumx(sliced)
    fx = Figure(histx)
    histy = sumy(sliced)
    fy = Figure(histy)
    Plot([[fx, fy], fig, sl])
    fig.render()
    fx.render()
    fy.render()
    sl._controls["zz"]["slider"].value = 10
