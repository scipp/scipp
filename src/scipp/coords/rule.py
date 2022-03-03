# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Rules encode instructions for how to compute a coordinate in ``transform_coords``.
They provide a common interface for renaming and computing new coordinates.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from copy import copy
import inspect

from typing import Any, Callable, Dict, Iterable, List, Mapping, Tuple

from ..core import Variable
from .coord import Coord, Destination

try:
    from typing import Protocol as _Protocol

    # Importing CoordTable from coord_table.py would result in an import
    # cycle because that module import rule.py
    # CoordTable is only needed for type annotations here,
    # so a protocol is enough.
    class _CoordProvider(_Protocol):
        def consume(self, name: str) -> Coord:
            pass

except ImportError:
    _Protocol = object
    _CoordProvider = Any


class Rule(ABC):
    def __init__(self, out_names: Tuple[str, ...]):
        self.out_names = out_names

    @abstractmethod
    def __call__(self, coords: _CoordProvider) -> Dict[str, Variable]:
        """Evaluate the rule."""

    @property
    @abstractmethod
    def dependencies(self) -> Tuple[str]:
        """Return names of coords that this rule needs as inputs."""

    def _format_out_names(self):
        return f'({", ".join(self.out_names)})'


class FetchRule(Rule):
    """
    Get coords from the provided dict-like sources.

    Can be used to abstract away retrieving coords and attrs from the input DataArray.
    """
    def __init__(self, out_names: Tuple[str, ...], dense_sources: Mapping[str,
                                                                          Variable],
                 event_sources: Mapping[str, Variable]):
        super().__init__(out_names)
        self._dense_sources = dense_sources
        self._event_sources = event_sources

    def __call__(self, coords: _CoordProvider) -> Dict[str, Coord]:
        return {
            out_name: Coord(dense=self._dense_sources.get(out_name, None),
                            event=self._event_sources.get(out_name, None),
                            destination=Destination.coord)
            for out_name in self.out_names
        }

    @property
    def dependencies(self) -> Tuple[str]:
        return ()  # type: ignore

    def __str__(self):
        return f'Input   {self._format_out_names()}'


class RenameRule(Rule):
    """
    Return the input coordinate and give it a new name.
    """
    def __init__(self, out_names: Tuple[str, ...], in_name: str):
        super().__init__(out_names)
        self._in_name = in_name

    def __call__(self, coords: _CoordProvider) -> Dict[str, Coord]:
        # Shallow copy the _Coord object to allow the alias to have
        # a different destination and usage count than the original.
        return {
            out_name: copy(coords.consume(self._in_name))
            for out_name in self.out_names
        }

    @property
    def dependencies(self) -> Tuple[str]:
        return tuple((self._in_name, ))

    def __str__(self):
        return f'Rename  {self._format_out_names()} <- {self._in_name}'


class ComputeRule(Rule):
    """
    Compute new coordinates using the provided callable.
    """
    def __init__(self, out_names: Tuple[str, ...], func: Callable):
        super().__init__(out_names)
        self._func = func
        self._arg_names = _arg_names(func)

    def __call__(self, coords: _CoordProvider) -> Dict[str, Coord]:
        inputs = {name: coords.consume(name) for name in self._arg_names}
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
        return self._without_unrequested(outputs)

    def _compute_pure_dense(self, inputs):
        outputs = self._func(**{name: coord.dense for name, coord in inputs.items()})
        outputs = self._to_dict(outputs)
        return {
            name: Coord(dense=var, event=None, destination=Destination.coord)
            for name, var in outputs.items()
        }

    def _compute_with_events(self, inputs):
        args = {
            name: coord.event if coord.has_event else coord.dense
            for name, coord in inputs.items()
        }
        outputs = self._to_dict(self._func(**args))
        # Dense outputs may be produced as side effects of processing event
        # coords.
        outputs = {
            name: Coord(dense=var if var.bins is None else None,
                        event=var if var.bins is not None else None,
                        destination=Destination.coord)
            for name, var in outputs.items()
        }
        return outputs

    def _without_unrequested(self, d: Dict[str, Any]) -> Dict[str, Any]:
        missing_outputs = [key for key in self.out_names if key not in d]
        if missing_outputs:
            raise TypeError(
                f'transform_coords was expected to compute {missing_outputs} '
                f'using `{self._func.__name__}` but the function returned '
                f'{list(d.keys())} instead.')
        return {key: d[key] for key in self.out_names}

    def _to_dict(self, output) -> Dict[str, Variable]:
        if not isinstance(output, dict):
            if len(self.out_names) != 1:
                raise TypeError('Function returned a single output but '
                                f'{len(self.out_names)} were expected.')
            return {self.out_names[0]: output}
        return output

    @property
    def dependencies(self) -> Tuple[str]:
        return self._arg_names

    @property
    def func_name(self) -> str:
        return self._func.__name__

    def __str__(self):
        return f'Compute {self._format_out_names()} = {self._func.__name__}' \
               f'({", ".join(self._arg_names)})'


def rules_of_type(rules: List[Rule], rule_type: type) -> Iterable[Rule]:
    yield from filter(lambda rule: isinstance(rule, rule_type), rules)


def rule_output_names(rules: List[Rule], rule_type: type) -> Iterable[str]:
    for rule in rules_of_type(rules, rule_type):
        yield from rule.out_names


def _arg_names(func) -> Tuple[str]:
    spec = inspect.getfullargspec(func)
    if spec.varargs is not None or spec.varkw is not None:
        raise ValueError('Function with variable arguments not allowed in '
                         f'conversion graph: `{func.__name__}`.')
    return tuple(spec.args + spec.kwonlyargs)
