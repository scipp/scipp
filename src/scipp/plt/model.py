# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .widgets.widget import WidgetView

from typing import Tuple, Iterable, Protocol, Tuple

from functools import partial

# class Model:
#     """
#     """

#     def __init__(self, data, notification_handler, name, notification_id=None):
#         self._data = data
#         self._graph = Model()
#         self._graph["root"] = Node("root")

#         self._notification_handler = notification_handler
#         self._name = name
#         self._filters = []
#         self._filtered_data = None
#         self._notification_id = notification_id

#     def add_filter(self, filt: Filter):
#         self._filters.append(filt)
#         if hasattr(filt, "register_callback"):
#             filt.register_callback(self.run)

#     def run(self):
#         self._filtered_data = self._data
#         for f in self._filters:
#             self._filtered_data = f(self._filtered_data)
#         if self._notification_id is not None:
#             self._notification_handler.notify_change({
#                 "name": self._name,
#                 "id": self._notification_id
#             })

#     def get_data(self):
#         return self._filtered_data

#     def get_coord(self, dim):
#         return self._data.meta[dim]

# class ModelCollection(dict):

#     def get_data(self, key):
#         return self[key].get_data()

#     def get_coord(self, key, dim):
#         return self[key].get_coord(dim)

#     def run(self):
#         for model in self.values():
#             model.run()


class Node:

    def __init__(self, func, name=None, views=None):
        self.name = name  # TODO do we need the name?
        self.parent_name = None
        self.dependency = None
        self.func = func
        self.views = []
        if views is not None:
            for view in views:
                self.views.append(view)

    # @property
    # def dependencies(self) -> Tuple[str]:
    #     return (self.dependency, )

    def send_notification(self, message):
        # print(f'hey there! from {self.name}: ', message)
        for view in self.views:
            view.notify({
                "node_name": self.name,
                "parent_name": self.parent_name,
                "message": message
            })

    def request_data(self):
        pipeline = []
        node = self
        while node.dependency is not None:
            pipeline.append(node.func)
            node = node.dependency
        else:
            pipeline.append(node.func)
        pipeline = list(pipeline[::-1])
        data = pipeline[0]()
        for step in pipeline[1:]:
            data = step(data)
        return data

    def add_view(self, view):
        self.views.append(view)


# class WidgetNode(Node):
#     def __init__(self, name, func, widgets):
#         super().__init__(name, func)
#         self._base_func = func  # func taking data array, dim, and int
#         self._widgets = widgets
#         self._callbacks = []
#         for widget in self._widgets.values():
#             widget.observe(self._update_func, names="value")
#         # widget.observe(self._update_func(), names="value")
#         self.add_view(widget)

#     @property
#     def values(self):
#         return {key: widget.value for key, widget in self._widgets.items()}

#     def _update_func(self, _):
#         self.func = partial(self._base_func, **self.values)
#         for callback in self._callbacks:
#             callback()

#     def register_callback(self, callback):
#         self._callbacks.append(callback)

#     def notify(self, _):
#         return


class Model:

    def __init__(self, da):
        self._name = da.name
        # root_node = Node(name='root', func=lambda: da)
        self._nodes = {}
        self["root"] = Node(name='root', func=lambda: da)
        # super().__init__({})

    # def __init__(self, nodes: Dict[str, Node]):
    #     self._nodes: Dict[str, Node] = nodes

    def __getitem__(self, name: str) -> Node:
        return self._nodes[name]

    def __setitem__(self, name: str, node: Node):
        self._nodes[name] = node
        node.parent_name = self._name
        node.name = name

    def items(self) -> Iterable[Tuple[str, Node]]:
        yield from self._nodes.items()

    # def parent_of(self, name: str) -> str:
    #     try:
    #         yield from self._nodes[name].dependency.name
    #     except KeyError:
    #         # Input nodes have no parents but are not represented in the
    #         # graph unless the corresponding FetchRules have been added.
    #         return

    def children_of(self, name: str) -> Iterable[str]:
        # for candidate, node in self.items():
        #     if name == node.dependency.name:
        #         yield candidate
        for node in self.values():
            if node.dependency is not None:
                if name == node.dependency.name:
                    yield node

    def keys(self) -> Iterable[str]:
        yield from self._nodes.keys()

    def values(self) -> Iterable[str]:
        yield from self._nodes.values()

    # def nodes_topologically(self) -> Iterable[str]:
    #     yield from TopologicalSorter(
    #         {out: node.dependencies
    #          for out, node in self.items()}).static_order()

    def insert(self, name: str, node: Node, after: str):
        assert after in self.keys()
        self[name] = node
        for child in self.children_of(after):
            child.dependency = node
        node.dependency = self[after]

    def notify_from_dependents(self, node: str):
        depth_first_stack = [node]
        while depth_first_stack:
            node = depth_first_stack.pop()
            self[node].send_notification('dummy message')
            for child in self.children_of(node):
                depth_first_stack.append(child.name)

    def add_view(self, key, view):
        self[key].add_view(view)
        view.add_model_node(self[key])
        if isinstance(view, WidgetView):
            view.add_notification(partial(self.notify_from_dependents, node=key))
            # view.add_model_node(self[key])

    # def request_data(self, name: str):
    #     pipeline = []
    #     node = self[name]
    #     while node.dependency is not None:
    #         pipeline.append(node.func)
    #         node = self[node.dependency]
    #     return pipeline[::-1]

    def end(self) -> Node:
        ends = []
        for node in self.values():
            # print(node.name)
            # print(tuple(self.children_of(node)))
            if len(tuple(self.children_of(node.name))) == 0:
                ends.append(node)
        if len(ends) != 1:
            raise RuntimeError(f'No unique end node: {ends}')
        return ends[0]

    # TODO dedup
    def show(self, size=None):
        dot = _make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for name, node in self.items():
            # name = output
            if node.dependency is not None:
                dot.edge(node.dependency.name, name)
            for view in node.views:
                key = str(view)
                dot.node(key, shape='ellipse', style='filled', color='lightgrey')
                dot.edge(name, key)

            # for arg in node.dependencies:
            #     if arg is None:
            #         continue
            #     dot.edge(arg, name)
        return dot

    def get_all_views(self):
        views = []
        for node in self.values():
            for view in node.views:
                views.append(view)
        return views


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)
