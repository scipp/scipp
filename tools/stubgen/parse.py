"""Parser for function docstrings."""

import ast
import inspect
import re
from typing import Any

from .config import CPP_CORE_MODULE_NAME, squash_overloads
from .transformer import (
    AddOverloadedDecorator,
    DropFunctionBody,
    OverwriteSignature,
    RemoveDecorators,
    SetFunctionName,
    add_decorator,
)


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
    return docstring.replace(
        "scipp::dataset::DataArray", f"{CPP_CORE_MODULE_NAME}.DataArray"
    ).replace("scipp::dataset::Dataset", f"{CPP_CORE_MODULE_NAME}.Dataset")


def _fix_default_arg_repr(docstring: str) -> str:
    """Docstrings show the repr of default args, this can include
    things that are not valid Python like '<automatically deduced unit>'."""
    return docstring.replace("<automatically deduced unit>", "default_unit")


def _make_docstring(doc: str) -> str:
    doc = doc.lstrip('\n')
    indent = " " * 8
    doc = '\n'.join(
        indent + line if line.strip() else line for line in doc.split('\n')
    ).strip()
    return f'        """{doc}\n        """'


def _make_code_for_parse_single(docstring: str, name: str) -> str:
    pieces = docstring.strip().split('\n\n', 1)
    if pieces[0].startswith('('):
        # Property docstrings don't contain the name.
        pieces[0] = name + pieces[0]
    elif not pieces[0].startswith(name):
        raise ValueError("Invalid docstring")
    signature = f'def {pieces[0]}:'
    if len(pieces) == 1 or not pieces[1].strip():
        return signature + ' ...'

    _, doc = pieces
    return f'{signature}\n{_make_docstring(doc)}'


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
        _make_code_for_parse_single('\n'.join(overloads[i : i + 2]), name)
        for i in range(0, len(overloads), 2)
    )


def _make_instancemethod_code_for_parse(docstring: str, name: str) -> str:
    docstring = _fix_cpp_namespace_access(docstring)
    docstring = _fix_default_arg_repr(docstring)
    if _is_overloaded(docstring):
        return _make_code_for_parse_overloaded(docstring, name)
    return _make_code_for_parse_single(docstring, name)


def _parse_instancemethod(func: object, name: str | None) -> list[ast.FunctionDef]:
    """Parse docstring of a method generated by Pybind11."""
    node = ast.parse(
        _make_instancemethod_code_for_parse(inspect.getdoc(func), name or func.__name__)
    )
    body = node.body
    body = squash_overloads(body)
    if len(body) > 1:
        transformer = AddOverloadedDecorator()
        return [transformer.visit(meth) for meth in body]
    return body


def _make_regular_method_code_for_parse(
    signature: inspect.Signature, name: str, docstring: str
) -> str:
    sig = f'def {name}{signature}:'
    if not docstring:
        return sig + ' ...'
    return f'{sig}\n{_make_docstring(docstring)}'


def _unwrap(func: Any) -> tuple[Any, int]:
    i = 0
    while hasattr(func, '__wrapped__'):
        i += 1
        func = func.__wrapped__
    return func, i


def _parse_regular_method(func: Any, name: str | None) -> list[ast.FunctionDef]:
    """Parse a function defined in Python."""
    func, n_decorators = _unwrap(func)
    node = ast.parse(inspect.getsource(func))
    node = DropFunctionBody().visit(node)
    node = RemoveDecorators(n_decorators).visit(node)
    if name is not None:
        node = SetFunctionName(name).visit(node)
    if (sig := getattr(func, '__signature__', None)) is not None:
        node = OverwriteSignature(sig).visit(node)
    node = ast.fix_missing_locations(node)
    return node.body


def parse_method(func: Any, name: str | None = None) -> list[ast.FunctionDef]:
    if inspect.isfunction(func):
        x = _parse_regular_method(func, name)
        return x
    return _parse_instancemethod(func, name)


def _make_property(prop: property, name: str, part: str) -> ast.FunctionDef | None:
    func = getattr(
        prop, {'getter': 'fget', 'setter': 'fset', 'deleter': 'fdel'}[part], None
    )
    if func is None:
        return None

    if part == 'getter':
        deco = ast.Name(id='property', ctx=ast.Load())
    else:
        deco = ast.Attribute(
            value=ast.Name(id=name, ctx=ast.Load()), attr=part, ctx=ast.Load()
        )

    (meth,) = parse_method(func, name)
    return add_decorator(meth, deco)


def parse_property(prop: property, name: str) -> list[ast.FunctionDef]:
    return list(
        filter(
            lambda p: p is not None,
            (
                _make_property(prop, name=name, part=part)
                for part in ('getter', 'setter', 'deleter')
            ),
        )
    )
