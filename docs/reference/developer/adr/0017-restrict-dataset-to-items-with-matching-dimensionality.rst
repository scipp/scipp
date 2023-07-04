ADR 0017: Restrict Dataset to items with matching dimensionality
================================================================

- Status: proposed
- Deciders: Jan-Lukas, Neil, Simon, Sunyoung
- Date: 2023-07-04

Context
-------

Background
~~~~~~~~~~

``Dataset`` was conceived as a way to represent a collection of items with compatible dimension extents, but possibly different sets or subsets of dimensions.
Recently we have introduced ``DataGroup``, which drops the restriction of compatible dimension extents but, unlike ``Dataset``, does in turn not provide support for joint coordinates.
The addition of ``DataGroup`` was triggered by a long series of cases where we realized that ``Dataset`` is not useful and flexbile enough in real applications.
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

Concrete problems:

- Reduction operations (e.g., ``sum()``) are not well-defined.
  We currently raise if there is a lower-dimensional item when a concrete reduction dim is provided, but support reducing all dimensions.
- Complicated logic for selecting coordinates.
  If a dataset has a dimension of length 1, should items without that dim have the coordinate?
- Slicing a dim can make coords suddenly apply for an item.
- Readonly flags in ``DataArray`` are required to avoid modifying data in lower-dimensional items via slices of a dataset.

- https://github.com/scipp/scipp/issues/3148 and https://github.com/scipp/scipp/issues/3149

``Dataset.dim`` may "lie" if an item is 0-D.
Need to distinguish between ordered and unordered dimensions for data array vs. dataset.
Should we still allow storing transposed items?
Maybe not.
We should probably set the dataset dims on creation?
But how do support empty dataset?
Keep uninitialized dataset dims, and set only when inserting first item?

Complicated updated of "sizes" dict.

.. code::

  import scipp as sc

  ds = sc.Dataset(dict(a=sc.arange("x", 4), b=sc.scalar(3)))
  ds[1] + ds[2]

There is no indication that 'b' is added to itself.
Note that ``DataGroup`` has the same shortcoming.

Summation and item access do not commute: ``sum()`` treats item as scalar, whereas other ops treat it as constant along dims:

.. code::

  import scipp as sc

  ds = sc.Dataset(dict(a=sc.arange("x", 4), b=sc.scalar(3)))
  print((ds.sum()['a'] + ds.sum()['b']).value)
  print((ds['a'] + ds['b']).sum().value)

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

If we had ``DataArray`` and ``DataGroup``, would we add ``Dataset`` in its current form and shape?
Probably not.
We might just add something to provide table-like data, but not necessarily with the more complex semantics as ``Dataset``.

There a two possible ways of looking at this:
Firstly, we may argue that while technically complex, the work has already been done, and the problems are encountered only in edge cases.
Secondly, we can ask ourselves if we would have added ``Dataset`` in its current form and shape if we had ``DataArray`` and ``DataGroup``.

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
  We could ignore this issues (as this is unlikely to be have been used in practice), or return a ``DataGroup`` instead.