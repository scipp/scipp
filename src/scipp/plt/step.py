class Step:
    def __init__(self, func, widget):
        self.func = func
        self.widget = widget

    def values(self):
        return self.widget.values()

    def register_callback(self, func):
        self.widget.set_callback(func)
