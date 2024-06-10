import ast
import enum
import inspect
from collections.abc import Iterable
from string import Template

from .config import (
    DISABLE_TYPE_CHECK_OVERRIDE,
    HEADER,
    INCLUDE_DOCS,
    TEMPLATE_FILE,
    class_is_excluded,
)
from .parse import parse_method, parse_property
from .transformer import RemoveDocstring, fix_method, fix_property


def _format_dunder_all(names):
    return (
        '__all__ = [\n    ' + ',\n    '.join('"' + name + '"' for name in names) + '\n]'
    )


def _add_suppression_comments(code: str) -> str:
    def _add_override(s: str) -> str:
        for name in DISABLE_TYPE_CHECK_OVERRIDE:
            if name in s:
                return s + '  # type: ignore[override, unused-ignore]'
        return s

    return '\n'.join(_add_override(line) for line in code.splitlines())


def _build_method(cls: type[type], method_name: str) -> [ast.FunctionDef]:
    meth = inspect.getattr_static(cls, method_name)
    return [
        fix_method(m, cls_name=cls.__name__) for m in parse_method(meth, method_name)
    ]


def _build_property(cls: type[type], property_name: str) -> [ast.FunctionDef]:
    prop = inspect.getattr_static(cls, property_name)
    return [fix_property(p) for p in parse_property(prop, property_name)]


def _build_attr(cls: type[type], attr_name: str) -> [ast.Expr]:
    typ: str = ast.parse(type(getattr(cls, attr_name)).__name__).body[0].value.id
    return [
        ast.AnnAssign(
            target=ast.Name(id=attr_name, ctx=ast.Store()),
            annotation=ast.Name(id=typ, ctx=ast.Load()),
            value=ast.Constant(value=Ellipsis),
            simple=1,
        )
    ]


class _Member(enum.Enum):
    function = enum.auto()
    instancemethod = enum.auto()
    prop = enum.auto()
    attr = enum.auto()
    skip = enum.auto()


def _classify(obj: object, member_name: str, cls: type[type]) -> _Member:
    if 'pybind11_static_property' in repr(inspect.getattr_static(cls, member_name)):
        return _Member.attr
    if inspect.isbuiltin(obj):
        return _Member.skip
    if inspect.isdatadescriptor(obj):
        return _Member.prop
    if inspect.isfunction(obj):
        return _Member.function
    if inspect.isroutine(obj) and 'instancemethod' in repr(obj):
        return _Member.instancemethod
    return _Member.skip


def _get_bases(cls: type[type]) -> list[ast.Name]:
    bases = [
        ast.Name(id=base.__name__)
        for base in cls.__bases__
        if 'pybind11' not in repr(base)
    ]
    # Bindings do not set base classes for mappings, so inject them manually.
    if cls.__name__ in ('Coords', 'Masks'):
        bases.append(ast.Name(id='Mapping[str, Variable]'))
    if cls.__name__ == 'Dataset':
        bases.append(ast.Name(id='Mapping[str, DataArray]'))
    return bases


def _build_class(cls: type[type]) -> ast.ClassDef | None:
    print(f'Generating class {cls.__name__}')
    body = []
    if cls.__doc__ and INCLUDE_DOCS:
        body.append(ast.Expr(value=ast.Constant(value=cls.__doc__)))

    for member_name, member in inspect.getmembers(cls):
        member_class = _classify(member, member_name, cls)
        if member_class == _Member.skip:
            continue
        if member_class == _Member.prop:
            code = _build_property(cls, member_name)
        elif member_class == _Member.attr:
            code = _build_attr(cls, member_name)
        else:
            code = _build_method(cls, member_name)
        body.extend(code)

    if not body:
        body.append(ast.Expr(value=ast.Constant(value=Ellipsis)))

    cls = ast.ClassDef(
        name=cls.__name__,
        bases=_get_bases(cls),
        keywords=[],
        decorator_list=[],
        body=body,
    )

    if not INCLUDE_DOCS:
        cls = ast.fix_missing_locations(RemoveDocstring().visit(cls))

    return ast.fix_missing_locations(cls)


def _cpp_classes() -> Iterable[type[type]]:
    from scipp._scipp import core

    for name, cls in inspect.getmembers(core, inspect.isclass):
        if not class_is_excluded(name):
            yield cls


def generate_stub() -> str:
    classes = [cls for cls in map(_build_class, _cpp_classes()) if cls is not None]
    classes_code = '\n\n'.join(map(ast.unparse, classes))
    classes_code = _add_suppression_comments(classes_code)

    with TEMPLATE_FILE.open('r') as f:
        templ = Template(f.read())

    return templ.substitute(
        header=HEADER,
        classes=classes_code,
        dunder_all=_format_dunder_all(cls.name for cls in classes),
    )
