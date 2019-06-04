.. _slicing:

Slicing
=======

Data in a :py:class:`~scippy.Variable` or :py:class:`~scippy.Dataset` can be indexed in a similar manner to NumPy and xarray.
The dimension to be sliced is specified using a dimension label and, in contrast to NumPy, positional dimension lookup is not available.
Positional indexing with an integer or an integer range is implemented by the ``slice`` method, which is available for variables, datasets, as well as items of a dataset.
In all cases a *view* is returned, i.e., just like when slicing a :py:class:`numpy.ndarray` no copy is performed.
