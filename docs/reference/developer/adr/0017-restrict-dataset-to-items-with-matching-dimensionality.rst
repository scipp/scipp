ADR 0017: Restrict Dataset to items with matching dimensionality
================================================================

- Status: proposed
- Deciders: Jan-Lukas, Neil, Simon, Sunyoung
- Date: 2023-06-29

Context
-------

Background
~~~~~~~~~~

``Dataset`` was conceived as a way to represent a collection of items with compatible dimension extents, but possibly different sets or subsets of dimensions.
Recently we have introduced ``DataGroup``, which drops the restriction of compatible dimension extents but, unlike ``Dataset``, does in turn not provide support for joint coordinates.
The addition of ``DataGroup`` was triggered by a long series of cases where we realized that ``Dataset`` is not useful and flexbile enough in many applications.
This is not to say that ``Dataset`` is entirely useless, but it is not as useful as we had hoped.
One key area that is not covered by ``DataGroup`` is the representation of table-like data (or multi-dimensional generalizations thereof), in a manner similar to ``pandas.DataFrame``.

The recent decision to remove support for ``attrs``, including addition of an ``aligned`` flag, has made the situation worse, as this added a number of edge cases where the semantics of ``Dataset`` are not clear.

Finally, there have been discussions around structure-of-array data-types.
``Dataset`` could be used for this purpose, but the current semantics may be too complex for comfort.

Analysis
~~~~~~~~

Certain meaningful applications cannot be handled.
But what are concrete examples?
If one item has lower dimensionality than another, what is the meaning of the data in the lower-dimensional item?
``Dataset`` takes the stance that this means the value is constant along the missing dimensions.
However, in practice this may actually not be true.
For example, imagine a 2-D temperature array and a 3-D pressure array.
Was the temperature measured only at ground level?
Then we should maybe not use ``Dataset``.
In this context, having ``Dataset`` with support for lower-dimensional items can be seen as risky, as it may lead to incorrect data analysis.

Proposed solution
~~~~~~~~~~~~~~~~~

We propose to restrict ``Dataset`` to items with matching dimensionality.
Each item of a dataset will retain a ``masks`` dictionary (no change).

Should we do this change under a new name, such as ``scipp.DataFrame``?
We believe that we currently have relatively few users of ``Dataset``, so in either case a migration would be simple.
Keeping the name ``Dataset`` would potentially be confusing for ``xarray`` users.
However ``DataFrame`` could also add confusion since items can be multi-dimensional.

Use ``DataArray`` with extra dimension if:

- Columns/items have the same ``dtype``.
- Column/item removal is not required.

Use ``Dataset`` if:

- Columns/items have different ``dtype``.
- Column/item removal is required.

User ``DataGroup`` if:

- Columns/items have different dimensionality or dimension sizes.

Decision
--------

Consequences
------------

Positive:
~~~~~~~~~

- Cleaner and simpler semantics.
- Simplified documentation and training.
- Simplified code.
- Resolves a number of issues where semantics of ``Dataset`` are not clear in edge-cases, in particular with bin-edge coordinates.

Negative:
~~~~~~~~~

- ``Dataset`` will no longer be able to represent certain types of data.
  Users will need to resort to ``DataGroup`` instead, which has other limitations, such as requiring to duplicate coordinates.
  Another option would be to replicate data values of the lower-dimensional items to match the dimensionality of the higher-dimensional items.
  This would reuqire more memory, but would force the users to be explicit about the meaning of data they want to represent.
- User code needs to be migrated.
- Existing files with incompatible ``Dataset`` data will no longer be readable.
  We could ignore this issues (as this is unlikely to be used in practice), or return a ``DataGroup`` instead.