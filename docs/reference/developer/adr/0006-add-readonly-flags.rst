ADR 0006: Add read-only flags
=============================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Owen, Simon
- Date: 2021-05-05

Context
-------

There are a number of contexts where values of variables are conceptually "broadcast" (not necessarily using an actual ``broadcast`` operation) and are thus shared (not to be confused with sharing from the shallow-copy mechanism):

- Explicit ``broadcast`` operations.
- Masks of dimension ``N-M`` if data is of dimension ``N``.
  Each mask value is conceptually shared by all data values along the ``M`` missing dimensions in the mask.
- Coords along dimension ``dim0`` of slices of a data array along dimension ``dim1``.
  The coord values are conceptually shared by all slices.
- Items independent of ``dim`` in a dataset which is then sliced along ``dim``.
  These items are conceptually shared by all slices.
- Coords of items in a dataset.
  The coords are conceptually shared by all items.

In all of the above cases a subsequent in-place modification would silently affect other unrelated (sub)objects such as other slices or items of the same "parent" object.

This can be solved by marking the variables affected in these cases as "read-only".

A further problem arises in in-place binary operations such as ``array['dim0', 0] += other``.
If the right-hand-side in such an operation contains masks that are not present in the left-hand-side they are inserted into the left-hand-side ``masks`` dict.
In this example, ``other`` contained a mask ``'extra_mask'`` that is not present in ``array`` it would get inserted into ``array['dim0', 0].masks``.
Since slicing operations create *new* meta data dicts, ``'extra_mask'`` would get inserted into a *temporary* dict, and silently disappear after the operation.
This is effectively "unmasking" elements.

Note that a hypothetical mechanism that would insert the masks into the slice's parent's masks dict, ``array.masks`` would need to provide a mechanism for broadcasting and initializing this new mask for all other slices.
The complexity of such a mechanism does not appear justifiable given the minor advantages.

The problem of meta-data insertion into slices can be solved by marking the meta data dicts of slices as "read-only", which prevents item insertion.

Decision
--------

Add ``readonly`` flag to:

- ``Variable``
- Metadata dicts for ``coords``, ``masks``, and ``attrs``.

Operations fail rather than silently ignoring read-only flags of variables or metadata dicts.

Consequences
------------

Positive:
~~~~~~~~~

- Can prevent bad modifications of variables that are broadcast.
  This allows for using broadcasting safely in more cases.
- Can prevent modification of dataset coords via items (data arrays), which would unintentionally affect other data arrays in the dataset.
- Can prevent bad mask updates in in-place binary operations without requiring mask dims to match data dims.
- Can prevent silently dropping meta data in in-place binary operations on slices.

Negative:
~~~~~~~~~

No major downsides.

In rare cases users may want to get a data array from a dataset, ``item = ds['a']``, and modify a coordinate without copying data.
This would now require copying these coords by hand, e.g., ``item.coords['x'] = item.coords['x'].copy()``.
In practice this should be a rare issue and users may just copy the entire item ``item = ds['a'].copy()``.
