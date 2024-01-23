ADR 0001: Remove dataset masking support
========================================

- Status: accepted
- Deciders: Neil, Owen, Simon
- Date: 2020-07-30

Context
-------

Masks for individual items of a dataset (e.g., ``ds['a'].masks()``) were initially not supported and are being implemented right now (this is a must-have requirement).
However, this creates a number of issues that do not appear to be solvable:

- Item masks may shadow dataset masks, i.e., we would need to prevent adding items masks with the same name as the mask in the dataset.
- Operations on items would need to update the "global" mask of the dataset.
  This is logically wrong, e.g., ``ds['a'] += data_array_with_mask`` would need to modify the mask values of ``ds``, influencing *all* items of ``ds``.
  The alternative would be to update only the item masks.
  However, this leads to the shadowing problem, i.e., adding a mask ``'mask'`` to ``ds['a'].masks()`` would shadow ("hide") ``ds.masks()['mask']``.
  We can imagine mechanism that would "combine" ``ds.masks()['mask']`` with ``ds['a'].masks()['mask']`` when accessing the masks of ``'a'``, but this would be highly complicated and brittle and bring limitations such as not being able to modify these masks.
- ``DataArray.masks()`` is difficult to reason about: Do we need to distinguish between masks that are "item" masks and masks that would become "global" masks when inserted in a dataset?

Decision
--------

Remove masking support from ``Dataset``, i.e., ``ds.masks()`` is no longer supported.

Consequences
------------

Positive:
~~~~~~~~~

- Dataset implementation simplifies.
- Documentation and training needs reduced.
- We can still do everything we need via item masks.
- Clear meaning of ``DataArray.masks()``.

Negative:
~~~~~~~~~

- It is no longer possible to simply mask elements of all items in a dataset with a single mask.
  The simplest example would be masking a row in a table.
  Instead we would need to mask each row in each column.
- Certain workflows, such as an existing SANS workflow need to be slightly adapted.
  Previously this had made use of the mask of the detector data to also effect masking of the norm terms.
  Instead we need to add this masks for both data and norm separately.
  This is not a large problem and might even make the code more understandable.
