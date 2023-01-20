import ast
import enum
import inspect
from string import Template
from typing import Iterable, Optional, Type

from .config import TEMPLATE_FILE, class_is_excluded
from .parse import parse_method, parse_property
from .transformer import fix_method, fix_property


def _build_method(cls: Type[type], method_name: str) -> [ast.FunctionDef]:
    meth = inspect.getattr_static(cls, method_name)
    return [
        fix_method(m, cls_name=cls.__name__) for m in parse_method(meth, method_name)
    ]


def _build_property(cls: Type[type], property_name: str) -> [ast.FunctionDef]:
    prop = inspect.getattr_static(cls, property_name)
    return [fix_property(p) for p in parse_property(prop, property_name)]


class _Member(enum.Enum):
    function = enum.auto()
    instancemethod = enum.auto()
    prop = enum.auto()
    skip = enum.auto()


def _classify(obj: object) -> _Member:
    if inspect.isbuiltin(obj):
        return _Member.skip
    if inspect.isdatadescriptor(obj):
        return _Member.prop
    if inspect.isfunction(obj):
        return _Member.function
    if inspect.isroutine(obj) and 'instancemethod' in repr(obj):
        return _Member.instancemethod
    return _Member.skip


def _build_class(cls: Type[type]) -> Optional[ast.ClassDef]:
    # TODO
    print(cls)
    body = []
    if cls.__doc__:
        body.append(ast.Expr(value=ast.Constant(value=cls.__doc__)))

    for member_name, member in inspect.getmembers(cls):
        member_class = _classify(member)
        if member_class == _Member.skip:
            continue
        if member_class == _Member.prop:
            code = _build_property(cls, member_name)
        else:
            code = _build_method(cls, member_name)
        body.extend(code)

    if not body:
        return None

    return ast.ClassDef(
        name=cls.__name__,
        bases=[],
        keywords=[],
        decorator_list=[],
        body=body,
    )


def _cpp_classes() -> Iterable[Type[type]]:
    from scipp._scipp import core
    for name, cls in inspect.getmembers(core, inspect.isclass):
        if not class_is_excluded(name):
            yield cls


def format_dunder_all(names):
    return '__all__ = [\n    ' + ',\n    '.join('"' + name + '"'
                                                for name in names) + '\n]'


def generate_stub() -> None:
    classes = [cls for cls in map(_build_class, _cpp_classes()) if cls is not None]
    classes_code = '\n\n'.join(map(ast.unparse, classes))

    with TEMPLATE_FILE.open('r') as f:
        templ = Template(f.read())

    with open('cpp_classes.pyi', 'w') as f:
        f.write(
            templ.substitute(classes=classes_code,
                             dunder_all=format_dunder_all(cls.__name__
                                                          for cls in _cpp_classes())))
