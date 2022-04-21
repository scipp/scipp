# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .controller import Controller
from .. import DataArray
from .figure import Figure
from .model import Model
from .widgets import WidgetCollection, WidgetFilter

from typing import Dict


class PlotDict():
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an `_ipython_display_`
    representation.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    """
    def __init__(self, *args, **kwargs):
        self._items = dict(*args, **kwargs)

    def __getitem__(self, key):
        return self._items[key]

    def __len__(self):
        return len(self._items)

    def keys(self):
        return self._items.keys()

    def values(self):
        return self._items.values()

    def items(self):
        return self._items.items()

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return plot contents into a single VBocx container
        """
        import ipywidgets as ipw
        contents = []
        for item in self.values():
            if item is not None:
                contents.append(item._to_widget())
        return ipw.VBox(contents)

    def show(self):
        """
        """
        for item in self.values():
            item.show()

    def close(self):
        """
        Close all plots in dict, making them static.
        """
        for item in self.values():
            item.close()


class Plot:
    """
    """
    def __init__(self,
                 data_arrays: Dict[str, DataArray],
                 filters: list = None,
                 **kwargs):

        self._notification_handler = NotificationHandler()
        self._views = {"figure": Figure(**kwargs)}
        self._models = {
            key: Model(data=array,
                       name=key,
                       notification_handler=self._notification_handler)
            for key, array in data_arrays.items()
        }
        for key, view in self._views.items():
            self._notification_handler.register_view(key, view)

        self._widgets = WidgetCollection()
        # self._controller = Controller(models=self._models, view=self._view)

        if isinstance(filters, list):
            for f in filters:
                for model in self._models.values():
                    model.add_filter(f)
        if isinstance(filters, dict):
            for key, filter_list in filters.items():
                for f in filter_list:
                    self._models[key].add_filter(f)

        # if filters is not None:
        #     for f in filters:
        #         self._controller.add_filter(f)
        #         if isinstance(f, WidgetFilter):
        #             self._widgets.append(f)

    def notify_change(self, change):
        for view in self._views.values():
            view.notify_change(change)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        self._controller.render()
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        import ipywidgets as ipw
        return ipw.VBox([self._view._to_widget(), self._widgets._to_widget()])

    def close(self):
        """
        Send close signal to the view.
        """
        self._view.close()

    def redraw(self):
        """
        """
        self._controller.render()

    def show(self):
        """
        """
        self._controller.render()
        self._view.show()

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self._view.savefig(filename=filename)
