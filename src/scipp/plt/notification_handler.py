class NotificationHandler:
    def __init__(self):
        self._views = {}

    def register_view(self, key, view):
        self._views[key] = view

    def notify_change(self, change):
        for key, view in self._views.items():
            view.notify(change)
