# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Rules encode instructions for how to compute a coordinate in ``transform_coords``.
They provide a common interface for renaming and computing new coordinates.
"""

from __future__ import annotations

import inspect
from abc import ABC, abstractmethod
from collections.abc import Callable, Iterable, Mapping
from copy import copy
from functools import partial
from typing import TYPE_CHECKING, Any, TypeVar

from ..core import Variable
from .coord import Coord

if TYPE_CHECKING:
    from coord_table import CoordTable
else:
    CoordTable = Any

Kernel = Callable[..., Variable | dict[str, Variable]]
_T = TypeVar('_T')


class Rule(ABC):
    def __init__(self, out_names: tuple[str, ...]):
        self.out_names = out_names

    @abstractmethod
    def __call__(self, coords: CoordTable) -> dict[str, Coord]:
        """Evaluate the rule."""

    @property
    @abstractmethod
    def dependencies(self) -> tuple[str, ...]:
        """Return names of coords that this rule needs as inputs."""

    def _format_out_names(self) -> str:
        return f'({", ".join(self.out_names)})'


class FetchRule(Rule):
    """
    Get coords from the provided dict-like sources.

    Can be used to abstract away retrieving coords from the input DataArray.
    """

    def __init__(
        self,
        out_names: tuple[str, ...],
        dense_sources: Mapping[str, Variable],
        event_sources: Mapping[str, Variable],
    ):
        super().__init__(out_names)
        self._dense_sources = dense_sources
        self._event_sources = event_sources

    def __call__(self, coords: CoordTable) -> dict[str, Coord]:
        return {
            out_name: Coord(
                dense=self._dense_sources.get(out_name, None),
                event=self._event_sources.get(out_name, None),
                aligned=True,
            )
            for out_name in self.out_names
        }

    @property
    def dependencies(self) -> tuple[str, ...]:
        return ()

    def __str__(self) -> str:
        return f'Input   {self._format_out_names()}'


class RenameRule(Rule):
    """
    Return the input coordinate and give it a new name.
    """

    def __init__(self, out_names: tuple[str, ...], in_name: str):
        super().__init__(out_names)
        self._in_name = in_name

    def __call__(self, coords: CoordTable) -> dict[str, Coord]:
        # Shallow copy the _Coord object to allow the alias to have
        # a different alignment and usage count than the original.
        return {
            out_name: copy(coords.consume(self._in_name)) for out_name in self.out_names
        }

    @property
    def dependencies(self) -> tuple[str, ...]:
        return (self._in_name,)

    def __str__(self) -> str:
        return f'Rename  {self._format_out_names()} <- {self._in_name}'


class ComputeRule(Rule):
    """
    Compute new coordinates using the provided callable.
    """

    def __init__(self, out_names: tuple[str, ...], func: Kernel):
        super().__init__(out_names)
        self._func = func
        self._arg_names = _arg_names(func)

    def __call__(self, coords: CoordTable) -> dict[str, Coord]:
        inputs = {
            name: coords.consume(coord) for coord, name in self._arg_names.items()
        }
        outputs = None
        if any(coord.has_event for coord in inputs.values()):
            outputs = self._compute_with_events(inputs)
        if all(coord.has_dense for coord in inputs.values()):
            dense_outputs = self._compute_pure_dense(inputs)
            if outputs is None:
                outputs = dense_outputs
            else:
                for name, coord in dense_outputs.items():
                    outputs[name].dense = coord.dense
        if outputs is None:
            raise RuntimeError(f"{self._func} produced no outputs")
        return self._without_unrequested(outputs)

    def _compute_pure_dense(self, inputs: dict[str, Coord]) -> dict[str, Coord]:
        outputs = self._to_dict(
            self._func(**{name: coord.dense for name, coord in inputs.items()})
        )
        return {
            name: Coord(dense=var, event=None, aligned=True)
            for name, var in outputs.items()
        }

    def _compute_with_events(self, inputs: dict[str, Coord]) -> dict[str, Coord]:
        args = {
            name: coord.event if coord.has_event else coord.dense
            for name, coord in inputs.items()
        }
        outputs = self._to_dict(self._func(**args))
        # Dense outputs may be produced as side effects of processing event
        # coords.
        return {
            name: Coord(
                dense=None if var.is_binned else var,
                event=var if var.is_binned else None,
                aligned=True,
            )
            for name, var in outputs.items()
        }

    def _without_unrequested(self, d: dict[str, _T]) -> dict[str, _T]:
        missing_outputs = [key for key in self.out_names if key not in d]
        if missing_outputs:
            raise TypeError(
                f'transform_coords was expected to compute {missing_outputs} '
                f'using `{self._func.__name__}` but the function returned '
                f'{list(d.keys())} instead.'
            )
        return {key: d[key] for key in self.out_names}

    def _to_dict(self, output: dict[str, Variable] | Variable) -> dict[str, Variable]:
        if not isinstance(output, dict):
            if len(self.out_names) != 1:
                raise TypeError(
                    'Function returned a single output but '
                    f'{len(self.out_names)} were expected.'
                )
            return {self.out_names[0]: output}
        return output

    @property
    def dependencies(self) -> tuple[str, ...]:
        return tuple(self._arg_names)

    @property
    def func_name(self) -> str:
        return self._func.__name__

    def __str__(self) -> str:
        # Class instances defining __call__ as well as objects created with
        # functools.partial may/do not define __name__.
        name = getattr(self._func, '__name__', repr(self._func))
        return (
            f'Compute {self._format_out_names()} = {name}'
            f'({", ".join(self.dependencies)})'
        )


def rules_of_type(rules: list[Rule], rule_type: type) -> Iterable[Rule]:
    yield from filter(lambda rule: isinstance(rule, rule_type), rules)


def rule_output_names(rules: list[Rule], rule_type: type) -> Iterable[str]:
    for rule in rules_of_type(rules, rule_type):
        yield from rule.out_names


def _arg_names(func: Kernel) -> dict[str, str]:
    spec = inspect.getfullargspec(func)
    if spec.varargs is not None or spec.varkw is not None:
        raise ValueError(
            'Function with variable arguments not allowed in '
            f'conversion graph: `{func.__name__}`.'
        )
    if inspect.isfunction(func) or getattr(func, '__class__', None) == partial:
        args = spec.args
    else:
        # Strip off the 'self'. Objects returned by functools.partial are not
        # functions, but nevertheless do not have 'self'.
        args = spec.args[1:]
    names = tuple(args + spec.kwonlyargs)
    coords = getattr(func, '__transform_coords_input_keys__', names)
    return dict(zip(coords, names, strict=True))
