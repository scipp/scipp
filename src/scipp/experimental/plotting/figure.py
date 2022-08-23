# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config, DataArray
from .tools import fig_to_pngbytes
from .toolbar import Toolbar
from .mesh import Mesh
from .line import Line
from .parser import parse_line_args, parse_mesh_args
from ...utils import name_with_unit
from .view import View

import ipywidgets as ipw
import matplotlib.pyplot as plt
from typing import Any, Tuple


class SideBar:

    def __init__(self, children=None):
        self._children = children if children is not None else []

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return ipw.VBox([child._to_widget() for child in self._children])


class Figure(View):

    def __init__(
            self,
            *nodes,
            ax: Any = None,
            figsize: Tuple[float, ...] = None,
            title: str = "",
            # xlabel: str = None,
            # ylabel: str = None,
            grid: bool = False,
            vmin=None,
            vmax=None,
            norm='linear',
            aspect='auto',
            **kwargs):

        super().__init__(*nodes)

        self._fig = None
        self._closed = False
        # self._title = title
        self._ax = ax
        # self._xlabel = xlabel
        # self._ylabel = ylabel
        self._user_vmin = vmin
        self._user_vmax = vmax
        self._norm = norm
        self._kwargs = kwargs
        self._dims = {}

        self.toolbar = self._make_toolbar()

        self.left_bar = SideBar([self.toolbar])
        self.right_bar = SideBar()
        self.bottom_bar = SideBar()
        self.top_bar = SideBar()

        self._children = {}

        cfg = config['plot']
        if self._ax is None:
            if figsize is None:
                figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
            self._fig, self._ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
            self._fig.tight_layout()
            if self.is_widget():
                self._fig.canvas.toolbar_visible = False
        else:
            self._fig = self._ax.get_figure()

        self._ax.set_title(title)
        self._ax.set_aspect(aspect)
        self._ax.grid(grid)

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

    def _to_widget(self) -> ipw.Widget:
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
            self.top_bar._to_widget(),
            ipw.HBox([self.left_bar._to_widget(), canvas,
                      self.right_bar._to_widget()]),
            self.bottom_bar._to_widget()
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
        for key, child in self._children.items():
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
            # self._ax.set_xlabel(self._xlabel)
            # self._ax.set_ylabel(self._ylabel)
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
                            **parse_line_args(self._kwargs, name=new_values.name))
                self._children[key] = line
                self._legend += bool(line.label)
                self._dims["x"] = new_values.dim
                # if self._ylabel is None:
                self._ax.set_ylabel(name_with_unit(var=new_values.data, name=""))
                self.toolbar.members['toggle_yaxis_scale'].value = self._norm == 'log'
                # self._ax.set_yscale(self._norm)

            elif new_values.ndim == 2:
                self._children[key] = Mesh(ax=self._ax,
                                           data=new_values,
                                           vmin=self._user_vmin,
                                           vmax=self._user_vmax,
                                           norm=self._norm,
                                           **parse_mesh_args(self._kwargs,
                                                             name=new_values.name))
                self._dims.update({"x": new_values.dims[1], "y": new_values.dims[0]})
                self._ax.set_ylabel(
                    name_with_unit(var=new_values.meta[self._dims["y"]]))

            self._ax.set_xlabel(name_with_unit(var=new_values.meta[self._dims["x"]]))
            # if self._ylabel is None and ("y" in self._dims):
            #     self._ylabel = name_with_unit(var=new_values.meta[self._dims["y"]])

        else:
            self._children[key].update(new_values=new_values)

        self.draw()

    def render(self):
        for node in self._graph_nodes.values():
            new_values = node.request_data()
            self._update(new_values=new_values, key=node.id)
        self._autoscale()
        self._draw_canvas()
