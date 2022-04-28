from typing import Tuple

from ...utils.graph import Graph


class PipelineNode:

    def __init__(self, name):
        self.name = name  # TODO do we need the name?
        self.dependency = None

    @property
    def dependencies(self) -> Tuple[str]:
        return (self.dependency, )

    def send_notification(self, message):
        print(f'hey there! from {self.name}: ', message)


class PipelineGraph(Graph):

    def __init__(self):
        super().__init__({})

    def insert(self, name: str, node: PipelineNode, after: str):
        assert after in self.nodes()
        self[name] = node
        for child in self.children_of(after):
            self[child].dependency = name
        node.dependency = after

    def notify_from_dependents(self, node: str):
        depth_first_stack = [node]
        while depth_first_stack:
            node = depth_first_stack.pop()
            self[node].send_notification('dummy message')
            for child in self.children_of(node):
                depth_first_stack.append(child)

    def end(self) -> str:
        ends = []
        for node in self.nodes():
            if len(tuple(self.children_of(node))) == 0:
                ends.append(node)
        if len(ends) != 1:
            raise RuntimeError(f'No unique end node: {ends}')
        return ends[0]

    # TODO dedup
    def show(self, size=None):
        dot = _make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, rule in self._nodes.items():
            name = output
            for arg in rule.dependencies:
                if arg is None:
                    continue
                dot.edge(arg, name)
        return dot


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)
