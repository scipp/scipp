# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .figure import Figure
from .toolbar import Toolbar

from functools import partial
import ipywidgets as ipw


class WidgetView:
    def __init__(self, widgets):
        # super().__init__(name, func)
        self._base_func = None  # func  # func taking data array, dim, and int
        self._widgets = widgets
        self._notifications = []
        self._model_nodes = {}

        for widget in self._widgets.values():
            widget.observe(self._update_func, names="value")
        # widget.observe(self._update_func(), names="value")
        # self.add_view(widget)

    @property
    def values(self):
        return {key: widget.value for key, widget in self._widgets.items()}

    def _update_func(self, _):
        # TODO: handle more than one model?
        nodes = next(iter(self._model_nodes.values()))
        for node in nodes.values():
            node.func = partial(self._base_func, **self.values)
        for notification in self._notifications:
            notification()

    def register_notification(self, notification):
        self._notifications.append(notification)

    def add_model_node(self, node):
        if node.parent_name not in self._model_nodes:
            self._model_nodes[node.parent_name] = {}
            self._model_nodes[node.parent_name][node.name] = node
        # self._model_nodes[key] = node
        self._base_func = node.func

    def notify(self, _):
        return


# class SideBar:
#     def __init__(self, children=None):
#         self._children = children if children is not None else []

#     def _ipython_display_(self):
#         return self._to_widget()._ipython_display_()

#     def _to_widget(self):
#         return ipw.VBox([child._to_widget() for child in self._children])

# class View:
#     """
#     Base class for 1d and 2d figures, that holds matplotlib axes.
#     """
#     def __init__(self, **kwargs):

#         self.figure = Figure(**kwargs)
#         self.toolbar = Toolbar()

#         self.toolbar.add_button(name="home_view",
#                                 callback=self.figure.home_view,
#                                 icon="home",
#                                 tooltip="Autoscale view")
#         self.toolbar.add_togglebutton(name="pan_view",
#                                       callback=self.figure.pan_view,
#                                       icon="arrows",
#                                       tooltip="Pan")
#         self.toolbar.add_togglebutton(name="zoom_view",
#                                       callback=self.figure.zoom_view,
#                                       icon="search-plus",
#                                       tooltip="Zoom")
#         self.toolbar.add_togglebutton(name='toggle_xaxis_scale',
#                                       callback=self.figure.toggle_xaxis_scale,
#                                       description="logx")
#         self.toolbar.add_togglebutton(name="toggle_yaxis_scale",
#                                       callback=self.figure.toggle_yaxis_scale,
#                                       description="logy")
#         self.toolbar.add_button(name="transpose",
#                                 callback=self.figure.transpose,
#                                 icon="retweet",
#                                 tooltip="Transpose")
#         self.toolbar.add_button(name="save_view",
#                                 callback=self.figure.save_view,
#                                 icon="save",
#                                 tooltip="Save")

#         self.left_bar = SideBar([self.toolbar])
#         self.right_bar = SideBar()
#         self.bottom_bar = SideBar()
#         self.top_bar = SideBar()

#     def savefig(self, filename: str = None):
#         """
#         Save plot to file.
#         Possible file extensions are `.jpg`, `.png` and `.pdf`.
#         The default directory for writing the file is the same as the
#         directory where the script or notebook is running.
#         """
#         self.figure.savefig(filename)

#     def _ipython_display_(self):
#         """
#         IPython display representation for Jupyter notebooks.
#         """
#         return self._to_widget()._ipython_display_()

#     def _to_widget(self) -> ipw.Widget:
#         """
#         Convert the Matplotlib figure to a widget. If the ipympl (widget)
#         backend is in use, return the custom toolbar and the figure canvas.
#         If not, convert the plot to a png image and place inside an ipywidgets
#         Image container.
#         """
#         return ipw.VBox([
#             self.top_bar._to_widget(),
#             ipw.HBox([
#                 self.left_bar._to_widget(),
#                 self.figure._to_widget(),
#                 self.right_bar._to_widget()
#             ]),
#             self.bottom_bar._to_widget()
#         ])

#     def draw(self):
#         """
#         """
#         self.figure.draw()

#     def update(self, *args, **kwargs):
#         return self.figure.update(*args, **kwargs)
