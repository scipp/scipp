from .. import DataArray


class Model:
    """
    """
    def __init__(self, data, parent, name, filters=None):
        self._data = data
        self._parent = parent
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
        filt.register_callback(self._run)

    def _run(self):
        self._filtered_data = self._data
        for f in self._filters:
            self._filtered_data = f(self._filtered_data)
        # return out
        self._parent.notify_change({"name": self._name, "type": "data"})
