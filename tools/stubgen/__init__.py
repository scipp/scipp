import ast
import inspect
from typing import Type

import scipp as sc

from .parse import parse_method, parse_property
from .transformer import fix_method, fix_property


def _build_method(cls: Type[type], method_name: str) -> [ast.FunctionDef]:
    meth = inspect.getattr_static(cls, method_name)
    return [fix_method(m, cls_name=cls.__name__) for m in parse_method(meth)]


def _build_property(cls: Type[type], property_name: str) -> [ast.FunctionDef]:
    prop = inspect.getattr_static(cls, property_name)
    return [fix_property(p) for p in parse_property(prop, property_name)]


def _build_class(cls: Type[type]) -> ast.ClassDef:
    body = []
    if cls.__doc__:
        body.append(ast.Expr(value=ast.Constant(value=cls.__doc__)))

    for method_name in ('__add__', 'astype'):
        body.extend(_build_method(cls, method_name))

    for prop_name in ('shape', 'unit'):
        body.extend(_build_property(cls, prop_name))

    return ast.ClassDef(
        name=cls.__name__,
        bases=[],
        keywords=[],
        decorator_list=[],
        body=body,
    )


def generate_stub() -> None:
    print(ast.unparse(_build_class(sc.Variable)))
    #
    # import inspect
    # from .parse import _make_method_code_for_parse
    # doc = inspect.getdoc(sc.Variable.shape)
    # print(doc)
    # print('==========================')
    # p = sc.Variable.shape
    # print(dir(p))
    # print(p.fdel)
    # print(p.fset)
    #
    # print(inspect.getdoc(p.fget))
    #
