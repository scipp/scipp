# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

# from .controller import Controller
from .. import DataArray


class Plot:
    """
    """

    def __init__(self):
        # data_arrays: Dict[str, DataArray],
        # views: list = None,
        # filters: list = None,
        # **kwargs):

        # self._notification_handler = NotificationHandler()

        self._models = {}
        # ModelCollection({
        #     key: Model(data=array,
        #                name=key,
        #                notification_handler=self._notification_handler,
        #                notification_id="data")
        #     for key, array in data_arrays.items()
        # })

        # self._views = []
        # if views is not None:
        #     for view in views:
        #         self.add_view(view)
        # else:
        #     self.add_view(Figure(**kwargs))

        # if filters is not None:
        #     if isinstance(filters, list):
        #         filters = {key: filters for key in self._models}
        #     for key, filter_list in filters.items():
        #         for f in filter_list:
        #             self.add_filter(key, f)

    def add_model(self, key, model):
        # model = Model(data=data_array,
        #               name=key,
        #               notification_handler=self._notification_handler,
        #               notification_id=notification_id)
        self._models[key] = model

    # def add_filter(self, model, f):
    #     self._models[model].add_filter(f)
    #     if isinstance(f, WidgetFilter):
    #         self.add_view(f)

    # def add_view(self, view):
    #     view.register_models(self._models)
    #     self._views.append(view)
    #     self._notification_handler.add_view(view)

    def render(self):
        for model in self._models.values():
            model.notify_from_dependents("root")

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        self.render()
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        import ipywidgets as ipw
        views = []
        for model in self._models.values():
            views += model.get_all_views()
        return ipw.VBox([view._to_widget() for view in set(views)])

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
