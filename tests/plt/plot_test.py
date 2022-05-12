# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
from scipp.plt import Plot, Graph, Figure, widgets, Node
from ..factory import make_dense_data_array, make_dense_dataset
import matplotlib.pyplot as plt
import matplotlib
import ipywidgets as ipw

matplotlib.use('Agg')


def test_plot_single_1d_line():
    da = make_dense_data_array(ndim=1)
    m = Graph(da)
    fig = Figure()
    m.add_view(m.root.name, fig)
    p = Plot()
    p.add_graph("da", m)
    p.render()


def test_plot_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    m_a = Graph(ds['a'])
    m_b = Graph(ds['b'])
    fig = Figure()
    m_a.add_view(m_a.root.name, fig)
    m_b.add_view(m_b.root.name, fig)
    p = Plot()
    p.add_graph("a", m_a)
    p.add_graph("b", m_b)
    p.render()


def test_plot_2d_image():
    da = make_dense_data_array(ndim=2)
    m = Graph(da)
    fig = Figure()
    m.add_view(m.root.name, fig)
    p = Plot()
    p.add_graph("da", m)
    p.render()


def test_plot_2d_image_smoothing_slider():
    da = make_dense_data_array(ndim=2)
    m = Graph(da)
    from scipy.ndimage import gaussian_filter

    def smooth(da, sigma):
        out = da.copy()
        smoothed = sc.array(dims=da.dims,
                            values=gaussian_filter(da.values, sigma=sigma),
                            unit=da.unit)
        out.data = smoothed
        return out

    sl = ipw.IntSlider(min=1, max=10)
    smooth_node = Node(func=smooth)
    smooth_view = widgets.WidgetView(widgets={"sigma": sl})
    m.add("smooth", smooth_node, after=m.end.name)
    f = Figure()
    m.add_view("smooth", f)
    m.add_view("smooth", smooth_view)
    p = Plot()
    p.add_graph("da", m)
    p.render()
    sl.value = 5


def test_plot_2d_image_with_masks():
    da = make_dense_data_array(ndim=2)
    da.masks['m1'] = da.data < sc.scalar(0.0, unit='counts')
    da.masks['m2'] = da.coords['xx'] > sc.scalar(30., unit='m')
    masks_node = Node(func=widgets.hide_masks)
    masks_widget = widgets.MaskWidget(masks=da.masks)
    masks_view = widgets.WidgetView(widgets={"masks": masks_widget})
    m = Graph(da)
    m.add("hiding_masks", masks_node, after=m.end.name)
    f = Figure()
    m.add_view("hiding_masks", f)
    m.add_view("hiding_masks", masks_view)
    p = Plot()
    p.add_graph("da", m)
    p.render()
    masks_widget._all_masks_button.value = False


def test_plot_two_1d_lines_with_masks():
    ds = make_dense_dataset()
    ds['a'].masks['m1'] = ds['a'].coords['xx'] > sc.scalar(40.0, unit='m')
    ds['a'].masks['m2'] = ds['a'].data < ds['b'].data
    ds['b'].masks['m1'] = ds['b'].coords['xx'] < sc.scalar(5.0, unit='m')
    m_a = Graph(ds['a'])
    m_b = Graph(ds['b'])

    a_masks_node = Node(func=widgets.hide_masks)
    a_masks_widget = widgets.MaskWidget(masks=ds['a'].masks)
    a_masks_view = widgets.WidgetView(widgets={"masks": a_masks_widget})
    b_masks_node = Node(func=widgets.hide_masks)
    b_masks_widget = widgets.MaskWidget(masks=ds['b'].masks)
    b_masks_view = widgets.WidgetView(widgets={"masks": b_masks_widget})

    m_a.add("hiding_masks_for_a", a_masks_node, after=m_a.end.name)
    m_b.add("hiding_masks_for_b", b_masks_node, after=m_b.end.name)
    m_a.add_view("hiding_masks_for_a", a_masks_view)
    m_b.add_view("hiding_masks_for_b", b_masks_view)

    fig = Figure()
    m_a.add_view("hiding_masks_for_a", fig)
    m_b.add_view("hiding_masks_for_b", fig)
    p = Plot()
    p.add_graph("a", m_a)
    p.add_graph("b", m_b)
    p.render()
    a_masks_widget._all_masks_button.value = False
    b_masks_widget._all_masks_button.value = False


def test_plot_node_sum_data_along_y():
    da = make_dense_data_array(ndim=2)

    def squash(da):
        return da.sum('yy')

    m = Graph(da)
    m.add("sum_y", Node(func=squash), after=m.end.name)
    f1d = Figure()
    f2d = Figure()
    m.add_view(m.root.name, f2d)
    m.add_view("sum_y", f1d)
    p = Plot()
    p.add_graph("da", m)
    p.render()


def test_plot_slice_3d_cube():
    da = make_dense_data_array(ndim=3)
    slice_node = Node(func=widgets.slice_dims)
    slice_slider = widgets.SliceView(da, dims=['zz'])
    m = Graph(da)
    m.add("slice_dims", slice_node, after=m.end.name)
    f = Figure()
    m.add_view("slice_dims", f)
    m.add_view("slice_dims", slice_slider)
    p = Plot()
    p.add_graph("da", m)
    p.render()
    slice_slider._widgets["slices"]._controls["zz"]["slider"].value = 10


def test_plot_3d_image_slicer_with_connected_side_histograms():
    fig = plt.figure(figsize=(10, 5))
    ax1 = fig.add_axes([0.05, 0.05, 0.5, 0.65])
    ax2 = fig.add_axes([0.05, 0.7, 0.5, 0.25])
    ax3 = fig.add_axes([0.55, 0.7, 0.45, 0.25])

    da = make_dense_data_array(ndim=3)
    slice_node = Node(func=widgets.slice_dims)
    slice_slider = widgets.SliceView(da, dims=['zz'])
    m = Graph(da)
    m.add("slice_dims", slice_node, after=m.end.name)
    f = Figure(ax=ax1, cbar=False)
    m.add_view("slice_dims", f)
    m.add_view("slice_dims", slice_slider)

    def sumx(da):
        return da.sum('xx')

    histx_node = Node(func=sumx)
    m.add("histx", histx_node, after="slice_dims")
    hx = Figure(ax=ax2)
    m.add_view("histx", hx)

    def sumy(da):
        return da.sum('yy')

    histy_node = Node(func=sumy)
    m.add("histy", histy_node, after="slice_dims")
    hy = Figure(ax=ax3)
    m.add_view("histy", hy)

    p = Plot()
    p.add_graph("da", m)
    p.render()
    slice_slider._widgets["slices"]._controls["zz"]["slider"].value = 10
