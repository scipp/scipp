from docutils import nodes
from docutils.parsers.rst import Directive
from sphinx.util.docutils import SphinxDirective


class MethodLinker(SphinxDirective):
    def run(self):
        print("----------------------------")
        print(self.env)
        print("----------------------------")
        paragraph_node = nodes.paragraph(text='meth!')
        return [paragraph_node]


def setup(app):
    app.add_directive("boundmethod", MethodLinker)
    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
