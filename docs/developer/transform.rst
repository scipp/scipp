Transforming data with custom operations
========================================

Overview
--------

Scipp can apply custom operations to the elements of one or more variables.
This is essentially are more advanced version of ``std::transform``.

Two alternatives are provided:

- ``transform_in_place`` transforms its first argument.
- ``transform`` returns a new ``Variable``.

Both variants support:

- Automatic broadcasting and alignment based on dimension labels.
  This does also include sparse data, and operations mixing sparse and dense data are supported.
- Automatic propagation of uncertainties, provided that the user-provided operation is built from known existing operations.
- Operations between different data types, and operations that produce outputs with a new data type.

Note that ``transform`` and ``transform_in_place`` act only on the data elements and do **not** handle units.
