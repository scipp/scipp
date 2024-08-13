# ADR 0018: More explicit control of replaced dims in `bin` and `hist` functions

-   Status: accepted
-   Deciders: Simon, Jan-Lukas, Neil, Sunyoung
-   Date: 2024-07-12

## Context

### Background

Currently functions such as `bin` and `hist` derive their behavior from looking at whether the coordinate being used for the operation is a "dimension-coordinate" of the data array, i.e., a coordinate with its name matching a dimension of the data.
Recently we have seen more and more cases where this behavior leads to surprising and too implicit results.
To control the actual behavior a number of workarounds in user-code are required, using combinations of functions such as `flatten`, `group`, `rename_dims`, `drop_coords`, `transpose`, etc.
Furthermore, there are cases where the default behavior shows sub-optimal performance, which can be avoided using further workarounds.
All of this is cumbersome and error prone, so this ADR proposes a more predictable and explicit control of the dimension(s) to be replaced or added.

### Proposed solution

1. Add a `dim` argument to all `bin` and `hist` functions to specify the dimension(s) to be replaced.
2. The default `dim=None` means the dimension of the *coordinate(s)* used for the operation.
3. In case of binned data there can be cases without coordinate, since only the bin contents might have the relevant coordinate.
   In this case `dim=None` means `dim=()`, i.e., no dimension is replaced but a new one is added.

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
Internally, `dim=None` is handled as follows:

- Use `dim=da.coords[key].dims if key in da.coords else ()` for each key in `arg_dict` and `kwargs`.
- If there are multiple keys (in `arg_dict` and/or `kwargs`), the `dim` argument is defined as the union of all the `dim` values defined above.

### Analysis

With the above proposal, we get the following behavior for dense data:

| Data dims | Coord dims | Result dims | Old op | New op | Change | Comments (old op) |
|-----------|------------|-------------|--------|--------|------------|----------|
| (x,)      | (x,)       | (x,)    | `.hist(x=x)`       | same |&#x2705;|          |
| (x,)      | (x,)       | (y,)    | `.hist(y=y)`       | same |&#x2705;|          |
| (x,)      | (x,)       | (y,z)   | `.hist(y=y, z=z)`  | same |&#x2705;|          |
| (x,y)     | (y,)       | (z,)    | `.flatten(...)`<br>`.hist(z=z)` | `.hist(z=z,dim=data.dims)` |&#x274c;|          |
| (x,y)     | (x,y)      | (z,)    | `.drop_coords(...)`<br>`.transpose(...)`<br>`.flatten(...)`<br>`.hist(z=z)`       | `.hist(z=z)` |&#x274c;| can also `hist(...).sum(...)` but has memory problems |
| (x,y)     | (y,)       | (x,z)   | `.flatten(...)`<br>`.group(...)`<br>`.hist(z=z)` | `.hist(z=z)` |&#x274c;|          |
| (x,y)     | (x,y)      | (x,z)   | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;|          |

In the cases where workarounds such as `flatten` are required, we currently get exceptions with a direct call to `hist`.
That is, there do not appear to be a significant number of breaking changes, unless someone relied on those exceptions.

For binned data we obtain:

| Data dims | Coord dims | Result dims | Old op | New op | Change | Consistent with dense case |
|-----------|------------|-------------|--------|--------|------------|----------|
| (...)     | (...)      | (...) | `.hist()`     | same |&#x2705;| -    |
| (x,)      | (x,)       | (x,)  | `.hist(x=x)`   | same |&#x2705;| &#x2705;    |
| (x,)      | -          | (y,)  | `.rename_dims(x=y)`<br>`.hist(y=y)` | `.hist(y=y, dim='x')` |&#x274c;| - |
| (x,)      | (x,)       | (y,)  | `.rename_dims(x=y)`<br>`.hist(y=y)` | `.hist(y=y)` |&#x274c;|&#x2705;|
| (x,)      | -          | (x,y) | `.hist(y=y)`  | `.hist(y=y)` |&#x2705;|-     |
| (x,)      | (x,)       | (x,y) | `.hist(y=y)`  | `.hist(y=y, dim=())` |&#x274c;|&#x2705;     |
| (x,y)     | -          | (z,)  | `.bins.concat()`<br>`.hist(z=z)`    | `.hist(z=z,dim=data.dims)` |&#x274c;|- |
| (x,y)     | (y,)       | (z,)  | `.bins.concat()`<br>`.hist(z=z)`    | `.hist(z=z,dim=data.dims)` |&#x274c;|&#x2705; |
| (x,y)     | (x,y)      | (z,)  | `.bins.concat()`<br>`.hist(z=z)`    | `.hist(z=z)` |&#x274c;|&#x2705; |
| (x,y)     | -          | (x,z) | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;|- |
| (x,y)     | (y,)       | (x,z) | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z)` |&#x274c;|&#x2705; |
| (x,y)     | (x,y)      | (x,z) | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;|&#x2705; |

The same holds for `hist->bin` in all the above cases.
As for dense data, the actual number of breaking changes appears to be small or even negligible.

We can see that in both the dense and the binned case, the proposed solution represents no change over the current behavior in the typical cases.
It does however allow for removing a significant number of "expert" workarounds, and it allows for more explicit control of the operation.

### Alternatives

In the binned-data case for `dim=None` it was initially considered defining `dim=<internal-content-dim>` instead of using the dimensions of the outer coordinate.
This would have resulted in the following:

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
However, the solution chosen for the proposal is more consistent with the dense-data case.
The only real downside of the proposed solution appears to be that in some cases the `dim` argument can be omitted but adding/changing a coord to/of the outer data array will require adding a `dim` argument since otherwise the output dimensionality will change.

## Decision

Implement proposed solution for all variants of the `bin` and `hist` functions.

## Consequences

### Positive:

- More explicit syntax, instead of relying on operation dimension name matching name and dimension of existing coordinate.
- Avoid several workarounds that are currently required in user code for certain operations.
- Avoid even more workarounds for optimal performance in user code.
- Complexity is absorbed into Scipp.

### Negative:

- Need to handle more cases in Scipp itself, with extra need for maintenance and testing.
  As a lot of this has already been done in special cases elsewhere this should be manageable and should result in a net gain if we consider a wider scope than just Scipp itself.