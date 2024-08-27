ADR 0017: Restrict Dataset to items with matching dimensionality
================================================================

- Status: accepted
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

There are two possible ways of reasoning about ``Dataset``.
Firstly, we may argue that while technically complex, the work has already been done, and the problems detailed below are encountered only in edge cases.
Secondly, we can ask ourselves if we would have added ``Dataset`` in its current form and shape if we had ``DataArray`` and ``DataGroup``.

Concrete problems are:

- If an item in a ``Dataset`` lacks one or more dimensions of other items, ``Dataset`` takes the stance that the value is constant along the missing dimensions.
  For example, imagine a 2-D temperature array and a 3-D pressure array.
  Was the temperature measured only at ground level?
  Then we should maybe not use ``Dataset``, as the temperature is not actually height-independent.
  In this context, having ``Dataset`` with support for lower-dimensional items can be seen as risky, as it may lead to incorrect data analysis.
- If we consider two slices of a ``Dataset`` containing a lower-dimensional item "abc" then, e.g., addition of these slices will yield a ``Dataset`` containing the sum of "abc" with itself.
  There is no indication that this happens, and it may be surprising to the users.
- Reduction operations such as ``sum()`` are not truly well-defined.
  We currently raise if there is a lower-dimensional item when a concrete reduction dim is provided, but support reducing all dimensions.
  This is inconsistent and potentially not a good choice (note though that a similar problem applies to ``DataGroup``).
  This implies, e.g., that addition of two ``Dataset`` items does not commute with calling ``sum()`` on the ``Dataset``.
- Complicated logic for selecting coordinates, affecting code as well as documentation needs.
  If a dataset has a dimension of length 1, should items without that dim have the coordinate?
- Slicing a dim can make coords suddenly apply for an item.
- Readonly flags in ``DataArray`` are required to avoid modifying data in lower-dimensional items via slices of a dataset.
- Consistency issues after the introduction of the alignment flag: `issues selecting coords with length-2 bin edges #3148 <https://github.com/scipp/scipp/issues/3148>`_ and `disappearing unaligned coords #3149 <https://github.com/scipp/scipp/issues/3149>`_.
- ``Dataset.dim`` may "lie" if an item is 0-D.
  Maybe it should have raised unless all items are 1-D?
- ``dims`` and ``sizes`` of ``DataArray`` imply an order, but they do not for ``Dataset``.
  This leads to some code overhead and risk of confusion.
- Complicated (internal) logic for updating the ``sizes`` dict.
  This is not a problem for the user, except for rare edge cases where size-changing item replacements are not supported although they could be.

Given the long-term focus of the project, and the limited area of applicability of ``Dataset`` with its current semantics, we believe that it is worth considering a change.
While none of the above issues are major, they add up to a significant amount of complexity that may turn out hard to manage or justify in the long run.
Even in its current state several aspects of the above are not well-documented, neither for developers nor for users.

Proposed solution
~~~~~~~~~~~~~~~~~

We propose to restrict ``Dataset`` to items with matching dimensionality.
Each item of a dataset will retain a ``masks`` dictionary (no change).

We can then think of Scipp providing a natural cascade of objects:
Given a number of arrays that we would like to combine, if ``sizes`` and ``dtype`` are consistent use ``DataArray`` with an extra dimension.
If only ``sizes`` are consistent use ``Dataset``.
If ``sizes`` are not consistent use ``DataGroup``.
Note that there also other reasons for choosing ``Dataset`` over ``DataArray``, in particular for simple string-based column access and efficient addition of or removal of items.

Decision
--------

- ``Dataset`` will be restricted to items with matching dimensionality.

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
  This would require more memory, but would force the users to be explicit about the meaning of data they want to represent.
- User code needs to be migrated.
- Existing files with incompatible ``Dataset`` data will no longer be readable.
  We could ignore this issues (as this is unlikely to be have been used in practice), or return a ``DataGroup`` instead.
