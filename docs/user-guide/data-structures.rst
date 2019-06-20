.. _data-structures:

Data Structures
===============

Variable
--------

:py:class:`scippy.Variable` is a labeled multi-dimensional array.
It consists of:

- ``values``: the values of the array
- ``variances``: the (optional) variances of the array values
- ``dims``: the dimension names and sizes of for each axis of the array
- ``unit``: the physical unit of the values in the array

Note that variables, unlike :py:class:`xarray.DataArray`, do *not* have coordinate arrays.

:py:class:`~scippy.Variable` also supports a single *sparse* dimension.

.. _data-structures-dataset:

Dataset
-------

:py:class:`scippy.Dataset` is a dict-like container of labeled arrays.
Individual data items are accessed using a string as a dict key.
A data item is essentially a :py:class:`~scippy.Variable` object together with relevant coords and labels.

In addition to data items, a dataset has the following key properties:

- ``coords``: a dict-like container of coordinates for the dataset, accessed using a dimension label as dict key.
- ``labels``: a dict-like container of labels (auxiliary coordinates) for the dataset, accessed using a string as dict key.
- ``attrs``: a dict-like container of attributes for the dataset, accessed using a string as dict key.

Note that ``coords`` in scippy correspond to dimension-coordinates in xarray, whereas ``labels`` corresponds to non-dimension coordinates.
See also the `xarray documentation <http://xarray.pydata.org/en/stable/data-structures.html#coordinates>`_.

The key distinction between ``coords``, ``labels``, and ``attrs`` is that the former two are required to match in operations between multiple datasets whereas the latter one is not.
All three are internally a :py:class:`~scippy.Variable`, i.e., they have a physical unit and optionally variances.
