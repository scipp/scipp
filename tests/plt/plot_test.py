# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
from scipp.plt import Plot, Figure, widgets, Node
from ..factory import make_dense_data_array, make_dense_dataset
import matplotlib
import ipywidgets as ipw

matplotlib.use('Agg')


def test_plot_single_1d_line():
    da = make_dense_data_array(ndim=1)
    data_node = Node(func=lambda: da)
    fig = Figure()
    data_node.add_view(fig)
    fig.render()


def test_plot_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    node_a = Node(func=lambda: ds['a'])
    node_b = Node(func=lambda: ds['b'])
    fig = Figure()
    node_a.add_view(fig)
    node_b.add_view(fig)
    fig.render()


def test_plot_difference_of_two_1d_lines():
    ds = make_dense_dataset(ndim=1)
    node_a = Node(func=lambda: ds['a'])
    node_b = Node(func=lambda: ds['b'])
    node_c = Node(func=lambda a, b: a - b, parents={"a": node_a, "b": node_b})
    fig = Figure()
    node_a.add_view(fig)
    node_b.add_view(fig)
    node_c.add_view(fig)
    fig.render()


def test_plot_2d_image():
    da = make_dense_data_array(ndim=2)
    data_node = Node(func=lambda: da)
    fig = Figure()
    data_node.add_view(fig)
    fig.render()


def test_plot_2d_image_smoothing_slider():
    da = make_dense_data_array(ndim=2)
    data_node = Node(func=lambda: da)
    sl = ipw.IntSlider(min=1, max=10)
    sigma_node = Node(lambda: sl.value)
    sl.observe(sigma_node.notify_children, names="value")

    from scipy.ndimage import gaussian_filter

    def smooth(da, sigma):
        out = da.copy()
        out.values = gaussian_filter(da.values, sigma=sigma)
        return out

    smooth_node = Node(func=smooth, parents={"da": data_node, "sigma": sigma_node})

    fig = Figure()
    smooth_node.add_view(fig)
    Plot([fig, sl])
    fig.render()
    sl.value = 5


def test_plot_2d_image_with_masks():
    da = make_dense_data_array(ndim=2)
    da.masks['m1'] = da.data < sc.scalar(0.0, unit='counts')
    da.masks['m2'] = da.coords['xx'] > sc.scalar(30., unit='m')
    data_node = Node(func=lambda: da)
    widget = widgets.MaskWidget(da.masks)
    widget_node = Node(lambda: widget.value)
    widget.observe(widget_node.notify_children, names="value")
    masks_node = Node(func=widgets.hide_masks,
                      parents={
                          "data_array": data_node,
                          "masks": widget_node
                      })
    fig = Figure()
    masks_node.add_view(fig)
    Plot([fig, widget])
    fig.render()
    widget._all_masks_button.value = False


def test_plot_two_1d_lines_with_masks():
    ds = make_dense_dataset()
    ds['a'].masks['m1'] = ds['a'].coords['xx'] > sc.scalar(40.0, unit='m')
    ds['a'].masks['m2'] = ds['a'].data < ds['b'].data
    ds['b'].masks['m1'] = ds['b'].coords['xx'] < sc.scalar(5.0, unit='m')

    node_a = Node(func=lambda: ds['a'])
    node_b = Node(func=lambda: ds['b'])
    widget_a = widgets.MaskWidget(ds['a'].masks)
    widget_b = widgets.MaskWidget(ds['b'].masks)
    widget_node_a = Node(lambda: widget_a.value)
    widget_node_b = Node(lambda: widget_b.value)

    node_masks_a = Node(func=widgets.hide_masks,
                        parents={
                            "data_array": node_a,
                            "masks": widget_node_a
                        })
    node_masks_b = Node(func=widgets.hide_masks,
                        parents={
                            "data_array": node_b,
                            "masks": widget_node_b
                        })

    widget_a.observe(node_masks_a.notify_children, names="value")
    widget_b.observe(node_masks_b.notify_children, names="value")

    fig = Figure()
    node_masks_a.add_view(fig)
    node_masks_b.add_view(fig)

    Plot([fig, [widget_a, widget_b]])
    fig.render()
    widget_a._all_masks_button.value = False
    widget_b._all_masks_button.value = False


def test_plot_node_sum_data_along_y():
    da = make_dense_data_array(ndim=2)
    data_node = Node(func=lambda: da)
    sumy_node = Node(lambda x: x.sum('yy'), parents={"x": data_node})
    fig1 = Figure()
    fig2 = Figure()
    data_node.add_view(fig1)
    sumy_node.add_view(fig2)
    Plot([[fig1, fig2]])
    fig1.render()
    fig2.render()


def test_plot_slice_3d_cube():
    da = make_dense_data_array(ndim=3)

    data_node = Node(func=lambda: da)
    sl = widgets.SliceWidget(da, ['zz'])
    input_node = Node(lambda: sl.value)
    sl.observe(input_node.notify_children, names="value")
    slice_node = Node(func=widgets.slice_dims,
                      parents={
                          "data_array": data_node,
                          "slices": input_node
                      })
    slice_node.add_view(sl)
    fig = Figure()
    slice_node.add_view(fig)
    Plot([fig, sl])
    fig.render()
    sl._controls["zz"]["slider"].value = 10


def test_plot_3d_image_slicer_with_connected_side_histograms():
    da = make_dense_data_array(ndim=3)
    data_node = Node(func=lambda: da)

    sl = widgets.SliceWidget(da, ['zz'])
    input_node = Node(lambda: sl.value)
    sl.observe(input_node.notify_children, names="value")

    slice_node = Node(func=widgets.slice_dims,
                      parents={
                          "data_array": data_node,
                          "slices": input_node
                      })

    f = Figure()
    slice_node.add_view(f)

    histx_node = Node(func=lambda da: da.sum('xx'), parents={"da": slice_node})
    hx = Figure()
    histx_node.add_view(hx)

    histy_node = Node(func=lambda da: da.sum('yy'), parents={"da": slice_node})
    hy = Figure()
    histy_node.add_view(hy)

    Plot([[hx, hy], f, sl])
    f.render()
    hx.render()
    hy.render()
    sl._controls["zz"]["slider"].value = 10
