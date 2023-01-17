"""Transform an AST to fix type annotations."""

import ast
from typing import Optional, Union

from .config import CORE_MODULE_NAME


def unqualified_cpp_class(node: Union[ast.Attribute, ast.Name]) -> Optional[str]:
    """Return the unqualified name of a C++ class in _scipp.core.
    Returns None if the class is not in _scipp.core.
    ``node`` should be a type annotation.
    """
    pieces = []
    while isinstance(node, ast.Attribute):
        pieces.append(node.attr)
        node = node.value
    pieces.append(node.id)
    if pieces[1:] != CORE_MODULE_NAME.split('.')[::-1]:
        return None
    return pieces[0]


def replace_returns(base: ast.FunctionDef,
                    returns: Union[ast.Name, ast.Attribute]) -> ast.FunctionDef:
    return ast.FunctionDef(name=base.name,
                           args=base.args,
                           body=base.body,
                           decorator_list=base.decorator_list,
                           returns=returns,
                           type_comment=base.type_comment)


def add_decorator(base: ast.FunctionDef,
                  decorator: Union[ast.Name, ast.Attribute]) -> ast.FunctionDef:
    return ast.FunctionDef(name=base.name,
                           args=base.args,
                           body=base.body,
                           decorator_list=[decorator, *base.decorator_list],
                           returns=base.returns,
                           type_comment=base.type_comment)


class AddOverloadedDecorator(ast.NodeTransformer):

    def visit_FunctionDef(self, node: ast.FunctionDef) -> ast.FunctionDef:
        self.generic_visit(node)
        return add_decorator(node,
                             decorator=ast.Attribute(value=ast.Name(id='typing',
                                                                    ctx=ast.Load()),
                                                     attr='overloaded',
                                                     ctx=ast.Load()))


class FixSelfArgName(ast.NodeTransformer):
    """Ensure that the first argument is called self."""

    def visit_arguments(self, node: ast.arguments) -> ast.arguments:
        self.generic_visit(node)
        keep = dict(kwonlyargs=node.kwonlyargs,
                    vararg=node.vararg,
                    kwarg=node.kwarg,
                    defaults=node.defaults)
        if node.posonlyargs and node.posonlyargs[0].arg != 'self':
            return ast.arguments(posonlyargs=[
                ast.arg('self', annotation=None, type_comment=None),
                *node.posonlyargs[1:]
            ],
                                 args=node.args,
                                 **keep)
        if not node.posonlyargs and node.args[0].arg != 'self':
            return ast.arguments(args=[
                ast.arg('self', annotation=None, type_comment=None), *node.args[1:]
            ],
                                 posonlyargs=node.posonlyargs,
                                 **keep)
        return node


class DropSelfAnnotation(ast.NodeTransformer):
    """Remove type annotations from 'self' argument."""

    def visit_arg(self, node: ast.arg) -> ast.arg:
        if node.arg != 'self':
            self.generic_visit(node)
            return node
        return ast.arg(node.arg, None, None)


class ShortenCppClassAnnotation(ast.NodeTransformer):
    """Remove module qualification from scipp classes."""

    def visit_Attribute(self, node: ast.Attribute) -> Union[ast.Attribute, ast.Name]:
        self.generic_visit(node)
        if (cls := unqualified_cpp_class(node)) is not None:
            return ast.Name(cls)
        return node


class FixObjectReturnType(ast.NodeTransformer):
    METHODS = (
        '__iadd__',
        '__iand__',
        '__iconcat__',
        '__ifloordiv__',
        '__ilshift__',
        '__imatmul__',
        '__imod__',
        '__imul__',
        '__ior__',
        '__ipow__',
        '__irshift__'
        '__isub__',
        '__itruediv__',
        '__ixor__',
    )

    def __init__(self, cls: Optional[str]) -> None:
        self.cls = cls

    def visit_FunctionDef(self, node: ast.FunctionDef) -> ast.FunctionDef:
        self.generic_visit(node)
        if node.name in self.METHODS and isinstance(
                node.returns, ast.Name) and node.returns.id == "object":
            return replace_returns(node, returns=ast.Name(self.cls))
        return node


class ObjectToAny(ast.NodeTransformer):

    def visit_Name(self, node: ast.Name) -> Union[ast.Name, ast.Attribute]:
        if node.id == 'object':
            return ast.Attribute(value=ast.Name(id='typing', ctx=ast.Load()),
                                 attr='Any',
                                 ctx=ast.Load())
        return node


def fix_method(node: ast.AST, cls_name: str) -> ast.AST:
    node = FixSelfArgName().visit(node)
    node = DropSelfAnnotation().visit(node)
    node = ShortenCppClassAnnotation().visit(node)
    node = FixObjectReturnType(cls_name).visit(node)
    node = ObjectToAny().visit(node)
    node = ast.fix_missing_locations(node)
    return node


def fix_property(node: ast.AST) -> ast.AST:
    node = FixSelfArgName().visit(node)
    node = DropSelfAnnotation().visit(node)
    node = ShortenCppClassAnnotation().visit(node)
    node = ObjectToAny().visit(node)
    node = ast.fix_missing_locations(node)
    return node
