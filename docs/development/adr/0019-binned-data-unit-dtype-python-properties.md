# ADR 0019: Binned data `unit` and `dtype` properties should return element unit and dtype.

-   Status: accepted
-   Deciders: Jan-Lukas, Neil, Simon
-   Date: 2025-02-04

## Context

### Background

Scipp supports *binned data*, where the array elements of a variable correspond to a table-like slice of a larger content table split into "bins".
The `unit` and `dtype` properties of a `DataArray` or `Variable` object return the unit and data type of the array as a whole.
In the case of binned data this is the "unit of the table" (`None`) and a "table dtype" (three cases, depending on whether the content table is given by a `Variable`, a `DataArray`, or a `Dataset`).

In practice, user code frequently needs access to the underlying unit or dtype of the "data" column of the table.
This has led various problems:

1. Bugs, from checking or operating with `obj.unit` or `obj.dtype`, which will break if unexpectedly a binned data `obj` is passed to this code.
2. Proliferation of code that needs to check for binned data and handle it differently, in particular functions such as `elem_unit` and `elem_dtype` that are used to access the unit and dtype of the "data" column of a binned data object, or return the regular unit and dtype if the object is not binned.

At the same time, the current `unit` and `dtype` properties are not useful for binned data, as they do not provide the information that is typically needed.
`unit` is always `None` and `dtype` is a table dtype which cannot be used for any other operation.

### Proposed solution

1. For binned variables, change the `unit` and `dtype` properties of `DataArray` and `Variable` to return the unit and dtype of the bin elements.
2. The `unit` setter should set the unit of the bin elements.
3. For the case of `Dataset` bin content, a unique unit and dtype are not defined.
   We should raise an exception in this case.
4. Make this change only in the Python bindings, not in the C++ core.

### Alternatives

- Add `elem_unit` and `elem_dtype` helper functions or properties to `DataArray` and `Variable` classes.
  This only solves parts of the problem, and makes the API larger and harder to use.
- For the proposed solution, in the case of `Dataset` bin content, it was considered to return `None` or a tuple of units/dtypes instead.
  This would be problematic for `mypy` and we have therefore decided against it.
  With the proposed solution of raising an exception we should still be in a position to revert this decision if it turns out to be insufficient.

## Decision

Binned data `unit` and `dtype` properties should return the element unit and dtype.
In the case of `Dataset` bin content, an exception should be raised.

## Consequences

### Positive:

- The API becomes more consistent and easier to use.
- Fewer bugs due to incorrect handling of binned data.
- Avoid confusing users with `DataArrayView` and `VariableView` dtype displayed, e.g., in `_repr_html_`.

### Negative:

- The change is not backwards compatible, but the impact is expected to be very small.
- Slight inconsistency in the `Dataset` bin content case.
  This is rarely used in practice.
