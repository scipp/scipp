# ADR 0018: More explicit control of replaced dims in `bin` and `hist` functions

-   Status: proposed
-   Deciders:
-   Date: 2024-07-11

## Context

### Background

### Proposed solution

1. Add `dim` argument to all `bin` and `hist` functions to specify the dimension(s) to be replaced.
2. The default `dim=None` means the dimension of the *coordinate* used for the operation.
3. In case of binned data, `dim=None` conceptually means the hidden internal content dimension.
   Coords of the outer data array are ignored for this purpose, if present.
   The alternative of using the dims of `da.bins.coords[dim]` is equivalent to using `da.dims`, again leading to many breaking changes and is not convenient.

Example of the new function signature:

```python
def hist(
    x: Variable | DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: tuple[str, ...] | str | None = None,
    **kwargs: int | Variable,
) -> DataArray: ...
```

That is, `dim` is keyword-only and can be a single string or a tuple of strings.

### Analysis

Dense data:

| Data dims | Coord dims | Result dims | Old op | New op | Change | Comments (old op) |
|-----------|------------|-------------|--------|--------|------------|----------|
| (x,)      | (x,)       | (x,)    | `.hist(x=x)`       | same |&#x2705;|          |
| (x,)      | (x,)       | (y,)    | `.hist(y=y)`       | same |&#x2705;|          |
| (x,)      | (x,)       | (y,z)   | `.hist(y=y, z=z)`  | same |&#x2705;|          |
| (x,y)     | (y,)       | (z,)    | `.flatten(...)`<br>`.hist(z=z)` | `.hist(z=z,dim=data.dims)` |&#x274c;|          |
| (x,y)     | (y,)       | (x,z)   | `.flatten(...)`<br>`.group(...)`<br>`.hist(z=z)` | `.hist(z=z)` |&#x274c;|          |
| (x,y)     | (x,y)      | (z,)    | `.drop_coords(...)`<br>`.transpose(...)`<br>`.flatten(...)`<br>`.hist(z=z)`       | `.hist(z=z)` |&#x274c;| can also `hist(...).sum(...)` but has memory problems |
| (x,y)     | (x,y)      | (x,z)   | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;|          |

In the cases where workarounds such as `flatten` are required, we currently get exceptions with a direct call to `hist`.
That is, there do not appear to be a significant number of breaking changes, unless someone relied on those exceptions.

Binned data:

| Data dims | Result dims | Old op | New op | Change | Comments (old op) |
|-----------|-------------|--------|--------|------------|----------|
| (...) | (...) | `.hist()`     | same |&#x2705;|     | |
| (x,)  | (x,)  | `.bin(x=x)`   | same |&#x2705;|     | |
| (x,)  | (x,)  | `.hist(x=x)`   | same |&#x2705;|     | |
| (x,)  | (y,)  | `.rename_dims(x=y)`<br>`.bin(y=y)`  | `.bin(y=y,dim='x')` |&#x274c;| Or use `.bin(y=y).bins.concat('x')` |
| (x,)  | (y,)  | `.rename_dims(x=y)`<br>`.hist(y=y)` | `.hist(y=y,dim='x')` |&#x274c;| Or use `.hist(y=y).sum('x')` |
| (x,)  | (x,y) | `.bin(y=y)`   | same |&#x2705;|     | |
| (x,)  | (x,y) | `.hist(y=y)`  | same |&#x2705;|     | |
| (x,y) | (x,z) | `.rename_dims(y=z)`<br>`.bin(z=z)`  | `.bin(z=z,dim='y')` |&#x274c;| Or use `.bin(z=z).bins.concat('y')` |
| (x,y) | (x,z) | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;| Or use `.bins.concat('y').hist(z=z)` |
| (x,y) | (z,)  | `.bins.concat()`<br>`.hist(z=z)`    | `.hist(z=z,dim=data.dims)` |&#x274c;| need extra tricks for multithreading |

There appears to be no breaking change here, since as before a new dimension will be added by `bin` or `hist`, with the default `dim=None`.


## Decision

## Consequences

### Positive:

- More explicit syntax, instead of relying on operation dimension name matching name and dimension of existing coordinate.
- Avoid several workarounds that are currently required in user code for certain operations.
- Avoid even more workarounds for optimal performance in user code.

### Negative:
