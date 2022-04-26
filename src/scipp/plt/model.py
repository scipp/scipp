from .filters import Filter


class Model:
    """
    """

    def __init__(self, data, notification_handler, name, notification_type="data"):
        self._data = data
        self._notification_handler = notification_handler
        self._name = name
        self._filters = []
        self._filtered_data = None
        self._notification_type = notification_type

    def add_filter(self, filt: Filter):
        self._filters.append(filt)
        if hasattr(filt, "register_callback"):
            filt.register_callback(self.run)

    def run(self):
        self._filtered_data = self._data
        for f in self._filters:
            self._filtered_data = f(self._filtered_data)
        self._notification_handler.notify_change({
            "name": self._name,
            "type": self._notification_type
        })

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
