"""This module checks that the stub file is consistent with the actual classes.

The tests are simple and only check for presence of classes and attributes.
They do not check whether function signatures match.
A combination of runtime tests and mypy should catch bad stub signatures.
"""

import ast
from pathlib import Path
from typing import cast

import pytest

import scipp as sc


def _name_participates(name: str) -> bool:
    # We don't check dunder attributes because
    # - Python defines a number of dunder attributes on all / most classes
    #   that don't need to be listed in the stub.
    # - They are usually (never) called directly on a concrete class.
    #   So a type checker is of limited use and we can live with discrepancies.
    return (
        not name.startswith('__')
        and not name.endswith('__')
        and name != '_pybind11_conduit_v1_'
    )


@pytest.fixture(scope='module')
def stub_source() -> str:
    return (
        Path(sc.__file__)
        .resolve()
        .parent.joinpath('core', 'cpp_classes.pyi')
        .read_text(encoding='utf-8')
    )


def _attr_names_from_ast(class_node: ast.ClassDef) -> set[str]:
    methods = {
        method.name
        for method in class_node.body
        if isinstance(method, ast.FunctionDef) and _name_participates(method.name)
    }
    # Assignments are, e.g., `DType.float64`.
    assignments = {
        cast(ast.Name, attr.target).id
        for attr in class_node.body
        if isinstance(attr, ast.AnnAssign)
        and _name_participates(cast(ast.Name, attr.target).id)
    }
    return methods | assignments


@pytest.fixture(scope='module')
def stub_classes(stub_source: str) -> dict[str, set[str]]:
    module = ast.parse(stub_source)
    return {
        node.name: _attr_names_from_ast(node)
        for node in module.body
        if isinstance(node, ast.ClassDef)
    }


def _attr_names_from_class(cls: type) -> set[str]:
    return {name for name, attr in cls.__dict__.items() if _name_participates(name)}


@pytest.fixture(scope='module')
def runtime_classes() -> dict[str, set[str]]:
    from scipp.core import cpp_classes

    return {
        cls.__name__: _attr_names_from_class(cls)
        for cls in cpp_classes.__dict__.values()
        if isinstance(cls, type)
    }


def test_compare_stub_classes_to_runtime(
    stub_classes: dict[str, set[str]], runtime_classes: dict[str, set[str]]
) -> None:
    assert stub_classes.keys() == runtime_classes.keys()
    for cls_name, runtime_attrs in runtime_classes.items():
        stub_attrs = stub_classes[cls_name]
        assert stub_attrs == runtime_attrs
