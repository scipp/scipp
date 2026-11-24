# Migrating after the switch to nanobind

## Overview

Scipp's Python bindings were re-implemented using [nanobind](https://nanobind.readthedocs.io/) (previously [pybind11](https://pybind11.readthedocs.io/)).

## Functions and methods

Functions and methods implemented in C++ are nanobind objects instead of built-in functions.
Repeated access to a bound method produces distinct method objects that compare unequal, for example `var.copy == var.copy` returns `False`.
They cannot be pickled or copied. Pickle the Scipp object and retrieve the method on the restored instance.

## Initialization

A second call to `__init__` on an initialized instance raises `TypeError`.
Subclasses that return a cached instance from `__new__` should call the base initializer only once, or cache instances in a factory.

## Iterators

Iterators over Scipp objects do not support `weakref.ref`.

## Interpreter shutdown

If Scipp objects are still alive when the interpreter exits, for example because a daemon thread holds them, nanobind reports them as leaked on stderr.

## Metadata views

A value obtained by iterating over `coords.values()`, `masks.values()` or `masks.items()` remains valid after its key is deleted or more entries are added.
Previously, it could refer to a different entry afterwards.

## Boolean scalars

A boolean scalar created from a float, for example with `sc.scalar(0.5, dtype=bool)`, takes the truth value of the float, as `sc.array` and NumPy do.
Previously, the float was truncated to an integer, so `0.5` gave `False`, and NaN and infinity raised an error.

## Element indexing

`values` that are not NumPy arrays, such as those of string or object variables, raise `IndexError` for any out-of-range index.
Negative indices count from the end when assigning, as they do when reading.
Previously, assigning with a negative or out-of-range index, or reading with a negative index beyond the start, accessed elements outside the view or memory outside the array.
