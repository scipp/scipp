ADR 0008: Consistent multi-dimensional coords
=============================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Owen, Simon
- Date: 2021-08-19

Context
-------

In the special case of non-dimension coords that have more than 1 dimension, they are considered to be labels for their inner dimension.
This causes trouble in many cases, e.g., after computation of ``phi`` and ``radius`` for 2-D data from ``x`` and ``y`` coords.
If for example ``phi.dims = ['x', 'y']`` and ``radius.dims = ['y', 'x']`` then an ``x`` slice will:

- Convert ``radius`` to an attribute because it is considered a coordinate of ``x``.
- Keep ``phi`` as a coordinate because it is considered a coordinate of ``y``.

However, for such coords it is just coincidence what its inner dimension is --- it isn't labelling either of the dims (or both, after slicing).

The conclusion seems to be that the current definition is wrong.
In the above example the expected outcome would be that radius and phi behave in the same manner, and that slicing along x or y behaves the same, i.e., either both or neither of radius and phi should turn into attributes.

A related limitation is that currently bin-edges must be edges for their inner dimension, because for dimension coords the matching coord key overrides which dimension the coord is associated with.
As a consequence such a bin-edge coord exceeds the data dims in a dimension other than its associated dimension, which is not valid, raising an exception.

Decision
--------

Do not associate multi-dimension coords with a dimension, unless:

- It is a bin-edge coord, in which case it gets associated with the bin-edge dimension.
- It is not a bin-edge coord, but a dimension coord, and gets associated with the coord key.

Consequences
------------

Positive:
~~~~~~~~~

- Fix inconsistent behavior.
- Bin-edges work as expected in more cases.
- Transposing is not necessary any more to work around limitations.
- Reduction operations now drop multi-dimensional coords that depend on the reduction dimension.
  This makes the operations more flexible while at the same time ensuring consistency, by dropping rather than preserving the coordinate.
  Note that in rare cases this may lead to undesirable consequences, e.g., if the result of the reduction is later added to another array where the coord is present, which would be wrong.

Negative:
~~~~~~~~~

- It is no longer possible to define an associated dimension for multi-dimensional coords that label a particular dimension.
