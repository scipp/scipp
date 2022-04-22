from .filters import Filter


class Model:
    """
    """
    def __init__(self, data, notification_handler, name, filters=None):
        self._data = data
        self._notification_handler = notification_handler
        self._name = name
        self._filters = []
        self._filtered_data = None

    def add_filter(self, filt: Filter):
        # if key is None:
        #     for pipeline in self._pipelines.values():
        #         pipeline.append(filt)
        #     filt.register_callback(self._run_all_pipelines)
        # else:
        self._filters.append(filt)
        if hasattr(filt, "register_callback"):
            filt.register_callback(self.run)

    def run(self):
        self._filtered_data = self._data
        for f in self._filters:
            self._filtered_data = f(self._filtered_data)
        # return out
        self._notification_handler.notify_change({"name": self._name, "type": "data"})

    def get_data(self):
        return self._filtered_data

    def get_coord(self, dim):
        return self._data.meta[dim]


class ModelCollection(dict):
    def get_data(self, key):
        return self[key].get_data()

    def get_coord(self, key, dim):
        return self[key].get_coord(dim)

    def run(self):
        for model in self.values():
            model.run()
