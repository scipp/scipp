# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config, DataArray
from .displayable import Displayable
from .tools import fig_to_pngbytes
from .toolbar import Toolbar
from .mesh import Mesh
from .line import Line
from ...utils import name_with_unit
from .view import View

import ipywidgets as ipw
import matplotlib.pyplot as plt
from typing import Any, Tuple


class SideBar(list, Displayable):

    def to_widget(self):
        return ipw.VBox([child.to_widget() for child in self])


class Figure(View):

    def __init__(self,
                 *nodes,
                 ax: Any = None,
                 figsize: Tuple[float, ...] = None,
                 title: str = "",
                 grid: bool = False,
                 vmin=None,
                 vmax=None,
                 norm='linear',
                 aspect='auto',
                 scale=None,
                 cbar=True,
                 errorbars=True,
                 mask_color='black',
                 crop=None,
                 **kwargs):

        super().__init__(*nodes)

        self._fig = None
        self._closed = False
        self._ax = ax
        self._user_vmin = vmin
        self._user_vmax = vmax
        self._norm = norm
        self._scale = {} if scale is None else scale
        self._crop = {} if crop is None else crop
        self._cbar = cbar
        self._errorbars = errorbars
        self._mask_color = mask_color
        self._kwargs = kwargs
        self._dims = {}
        self._children = {}

        cfg = config['plot']
        if self._ax is None:
            if figsize is None:
                figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
            self._fig, self._ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
            self._fig.tight_layout(rect=[0.05, 0.02, 1.0, 1.0])
            if self.is_widget():
                self._fig.canvas.toolbar_visible = False
                self._fig.canvas.header_visible = False
        else:
            self._fig = self._ax.get_figure()

        self._ax.set_title(title)
        self._ax.set_aspect(aspect)
        self._ax.grid(grid)

        self.left_bar = SideBar()
        self.right_bar = SideBar()
        self.bottom_bar = SideBar()
        self.top_bar = SideBar()

        self.toolbar = self._make_toolbar()
        if self.is_widget():
            self.left_bar.append(self.toolbar)

        self._legend = 0
        self._new_artist = False

    def _make_toolbar(self):
        toolbar = Toolbar()
        toolbar.add_button(name="home_view",
                           callback=self.home_view,
                           icon="home",
                           tooltip="Autoscale view")
        toolbar.add_togglebutton(name="pan_view",
                                 callback=self.pan_view,
                                 icon="arrows",
                                 tooltip="Pan")
        toolbar.add_togglebutton(name="zoom_view",
                                 callback=self.zoom_view,
                                 icon="search-plus",
                                 tooltip="Zoom")
        toolbar.add_togglebutton(name='toggle_xaxis_scale',
                                 callback=self.toggle_xaxis_scale,
                                 description="logx")
        toolbar.add_togglebutton(name="toggle_yaxis_scale",
                                 callback=self.toggle_yaxis_scale,
                                 description="logy")
        toolbar.add_button(name="save_view",
                           callback=self.save_view,
                           icon="save",
                           tooltip="Save")
        return toolbar

    def is_widget(self) -> bool:
        """
        Check whether we are using the Matplotlib widget backend or not.
        "on_widget_constructed" is an attribute specific to `ipywidgets`.
        """
        return hasattr(self._fig.canvas, "on_widget_constructed")

    def to_widget(self) -> ipw.Widget:
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        self.render()

        canvas = self._fig.canvas if (self.is_widget()
                                      and not self._closed) else self._to_image()

        return ipw.VBox([
            self.top_bar.to_widget(),
            ipw.HBox([self.left_bar.to_widget(), canvas,
                      self.right_bar.to_widget()]),
            self.bottom_bar.to_widget()
        ])

    def _to_image(self) -> ipw.Image:
        """
        Convert the Matplotlib figure to a static image.
        """
        width, height = self._fig.get_size_inches()
        dpi = self._fig.get_dpi()
        return ipw.Image(value=fig_to_pngbytes(self._fig),
                         width=width * dpi,
                         height=height * dpi)

    def _autoscale(self):
        global_xmin = None
        global_xmax = None
        global_ymin = None
        global_ymax = None
        xscale = self._ax.get_xscale()
        yscale = self._ax.get_yscale()
        for child in self._children.values():
            xmin, xmax, ymin, ymax = child.get_limits(xscale=xscale, yscale=yscale)
            if isinstance(child, Line):
                if self._user_vmin is not None:
                    ymin = self._user_vmin
                if self._user_vmax is not None:
                    ymax = self._user_vmax
            if global_xmin is None or xmin.value < global_xmin:
                global_xmin = xmin.value
            if global_xmax is None or xmax.value > global_xmax:
                global_xmax = xmax.value
            if global_ymin is None or ymin.value < global_ymin:
                global_ymin = ymin.value
            if global_ymax is None or ymax.value > global_ymax:
                global_ymax = ymax.value
        self._ax.set_xlim(global_xmin, global_xmax)
        self._ax.set_ylim(global_ymin, global_ymax)

    def draw(self):
        """
        """
        if self._new_artist:
            if self._legend > 0:
                self._ax.legend()
            self._new_artist = False
        self._draw_canvas()

    def _draw_canvas(self):
        self._fig.canvas.draw_idle()

    def home_view(self, *_):
        self._autoscale()
        self._draw_canvas()

    def pan_view(self, *_):
        self._fig.canvas.toolbar.pan()

    def zoom_view(self, *_):
        self._fig.canvas.toolbar.zoom()

    def save_view(self, *_):
        self._fig.canvas.toolbar.save_figure()

    def toggle_xaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_xscale(swap_scales[self._ax.get_xscale()])
        self._autoscale()
        self._draw_canvas()

    def toggle_yaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_yscale(swap_scales[self._ax.get_yscale()])
        self._autoscale()
        self._draw_canvas()

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self._fig.savefig(filename, bbox_inches="tight")

    def notify_view(self, message):
        node_id = message["node_id"]
        new_values = self._graph_nodes[node_id].request_data()
        self._update(new_values=new_values, key=node_id)

    def _update(self, new_values: DataArray, key: str):
        """
        Update image array with new values.
        """
        if new_values.ndim > 2:
            raise ValueError("Figure can only be used to plot 1-D and 2-D data.")
        if key not in self._children:
            self._new_artist = True
            if new_values.ndim == 1:
                line = Line(ax=self._ax,
                            data=new_values,
                            number=len(self._children),
                            **{
                                **{
                                    'errorbars': self._errorbars,
                                    'mask_color': self._mask_color
                                },
                                **self._kwargs
                            })
                self._children[key] = line
                self._legend += bool(line.label)
                self._dims['x'] = {
                    'dim': new_values.dim,
                    'unit': new_values.meta[new_values.dim].unit
                }
                self._ax.set_ylabel(name_with_unit(var=new_values.data, name=""))
                # TODO: this is not great, involves implementation details of the
                # toolbar. However, setting the yscale of the axis manually would mean
                # we need to update the value of the 'logy' button depending on the
                # value of self._norm, and updating that value would trigger a callback
                # which would draw the axes a second time.
                self.toolbar.members['toggle_yaxis_scale'].value = self._norm == 'log'

            elif new_values.ndim == 2:
                self._children[key] = Mesh(ax=self._ax,
                                           data=new_values,
                                           vmin=self._user_vmin,
                                           vmax=self._user_vmax,
                                           norm=self._norm,
                                           **{
                                               **{
                                                   'cbar': self._cbar,
                                               },
                                               **self._kwargs
                                           })
                self._dims.update({
                    "x": {
                        'dim': new_values.dims[1],
                        'unit': new_values.meta[new_values.dims[1]].unit
                    },
                    "y": {
                        'dim': new_values.dims[0],
                        'unit': new_values.meta[new_values.dims[0]].unit
                    }
                })
                if new_values.dims[0] in self._scale:
                    self.toolbar.members['toggle_yaxis_scale'].value = self._scale[
                        self._dims['y']]['dim'] == 'log'
                self._ax.set_ylabel(
                    name_with_unit(var=new_values.meta[self._dims['y']['dim']]))

            self._ax.set_xlabel(
                name_with_unit(var=new_values.meta[self._dims['x']['dim']]))
            if self._dims['x']['dim'] in self._scale:
                self.toolbar.members['toggle_xaxis_scale'].value = self._scale[
                    self._dims['x']['dim']] == 'log'

        else:
            self._children[key].update(new_values=new_values)

        self.draw()

    def render(self):
        for node in self._graph_nodes.values():
            new_values = node.request_data()
            self._update(new_values=new_values, key=node.id)
        self._autoscale()
        for dim, lims in self._crop.items():
            for xy in self._dims:
                if dim == self._dims[xy]['dim']:
                    getattr(self._ax, f'set_{xy}lim')(*[
                        lims[m].to(unit=self._dims[xy]['unit']).value
                        for m in ('min', 'max') if m in lims
                    ])
        self._draw_canvas()
