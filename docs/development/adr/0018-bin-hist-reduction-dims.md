# ADR 0018: More explicit control of replaced dims in `bin` and `hist` functions

-   Status: proposed
-   Deciders:
-   Date: 2024-07-11

## Context

### Background

### Analysis

| Data dims | Coord dims | Result dims | Old op | New op | Diff | Comments |
|-----------|------------|-------------|--------|--------|------------|----------|
| (x,)      | (x,)       | (y,)    | `.hist(y=y)`       | same |&#x2705;|          |
| (x,)      | (x,)       | (y,x)   | `.hist(y=y, x=x)`  | same |&#x2705;|          |
| (x,)      | (x,)       | (x,)    | `.hist(x=x)`       | same |&#x2705;|          |
| (x,y)     | (y,)       | (z,)   | `.flatten(...)`<br>`.hist(z=z)` | `.hist(z=z,dim=data.dims)` |&#x274c;|          |
| (x,y)     | (y,)       | (x,y)   | `.flatten(...)`<br>`.group(...)`<br>`.hist(y=y)` | `.hist(y=y)` |&#x274c;|          |
| (x,y)     | (x,y)      | (z,)    | `.drop_coords(...)`<br>`.transpose(...)`<br>`.flatten(...)`<br>`.hist(z=z)`       | `.hist(z=z)` |&#x274c;| can also `hist(...).sum(...)` but has memory problems |
| (x,y)     | (x,y)      | (x,z)   | `.rename_dims(y=z)`<br>`.hist(z=z)` | `.hist(z=z,dim='y')` |&#x274c;|          |

### Proposed solution

## Decision

## Consequences

### Positive:

### Negative:
