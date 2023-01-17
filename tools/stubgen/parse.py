"""Parser for function docstrings."""
import ast
import inspect
import re
from typing import List

from .config import CORE_MODULE_NAME
from .transformer import AddOverloadedDecorator


def _is_overloaded(docstring: str) -> bool:
    lines = docstring.split('\n')
    if len(lines) > 1 and 'Overloaded' in lines[1]:
        return True
    return False


def _fix_cpp_namespace_access(docstring: str) -> str:
    """Some docstrings contain C++-style namespace accesses,
    e.g. scipp::dataset::DataArray.
    Convert those to {CORE_MODULE_NAME}.DataArray
    """
    return (docstring.replace("scipp::dataset::DataArray",
                              f"{CORE_MODULE_NAME}.DataArray").replace(
                                  "scipp::dataset::Dataset",
                                  f"{CORE_MODULE_NAME}.Dataset"))


def _make_code_for_parse_single(docstring: str, name: str) -> str:
    pieces = docstring.strip().split('\n\n', 1)
    if not pieces[0].startswith(name):
        raise ValueError("Invalid docstring")
    signature = f'def {pieces[0]}:'
    if len(pieces) == 1 or not pieces[1].strip():
        return signature + ' ...'

    _, doc = pieces
    doc.lstrip('\n')
    indent = " " * 4
    doc = '\n'.join(indent + line if line.strip() else line
                    for line in doc.split('\n')).strip()
    return f'{signature}\n    """{doc}\n    """'


def _make_code_for_parse_overloaded(docstring: str, name: str) -> str:
    """Overloads have docstrings of the form

        foo(*args, **kwargs)
        Overloaded function.

        1. foo(self: scipp._scipp.core.Variable) -> scipp._scipp.core.Variable

        Description

        2. foo(self: scipp::dataset::DataArray) -> scipp::dataset::DataArray

        Description
    """
    # Regex to match the '1. foo(self: ...' lines
    pattern = re.compile(rf'\n\d+\.\s*({name}\(.*\)\s*->.+)')
    # split and drop stuff before first overload
    overloads = pattern.split(docstring)[1:]
    return '\n'.join(
        _make_code_for_parse_single('\n'.join(overloads[i:i + 2]), name)
        for i in range(0, len(overloads), 2))


def _make_method_code_for_parse(docstring: str, name: str) -> str:
    docstring = _fix_cpp_namespace_access(docstring)
    if _is_overloaded(docstring):
        return _make_code_for_parse_overloaded(docstring, name)
    return _make_code_for_parse_single(docstring, name)


def parse_method(func: object) -> List[ast.FunctionDef]:
    node = ast.parse(_make_method_code_for_parse(inspect.getdoc(func), func.__name__))
    if len(node.body) > 1:
        transformer = AddOverloadedDecorator()
        return [transformer.visit(meth) for meth in node.body]
    return node.body  # type: ignore


def _make_property(prop: property, name: str, part: str) -> str:
    func = getattr(prop, {
        'getter': 'fget',
        'setter': 'fset',
        'deleter': 'fdel'
    }[part], None)
    if func is None:
        return ''

    if part == 'getter':
        code = '@property'
    else:
        code = f'@{name}.{part}'

    code += f'\ndef {name}{inspect.getdoc(func)}:'
    if doc := inspect.getdoc(prop):
        code += f'\n    """{doc}"""'
    else:
        code += ' ...'

    return code


def parse_property(prop: property, name: str) -> List[ast.FunctionDef]:
    node = ast.parse('\n'.join(
        (_make_property(prop, name=name,
                        part='getter'), _make_property(prop, name=name, part='setter'),
         _make_property(prop, name=name, part='deleter'))))
    return node.body  # type: ignore
