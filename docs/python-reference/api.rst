.. currentmodule:: scipp

Classes
=======

.. autosummary::
   :toctree: ../generated

   Unit
   Variable
   VariableView
   DataArray
   DataArrayView
   Dataset
   DatasetView
   GroupByDataArray
   GroupByDataset
   Bins

Creation functions
==================

.. autosummary::
   :toctree: ../generated

   array
   scalar
   zeros
   ones
   empty

Free functions
==============

General
~~~~~~~

.. autosummary::
   :toctree: ../generated

   abs
   bin
   bins
   choose
   collapse
   dot
   exp
   filter
   histogram
   log
   log10
   merge
   norm
   rebin
   reciprocal
   slices
   sort
   sqrt
   stddevs
   to_unit
   values
   variances

Shape
~~~~~

.. autosummary::
   :toctree: ../generated

   broadcast
   concatenate
   flatten
   fold
   reshape
   transpose

Comparison
~~~~~~~~~~

Comparison operators compare element-wise and *ignore variances*.

.. autosummary::
   :toctree: ../generated

   less
   greater
   less_equal
   greater_equal
   equal
   not_equal
   isclose
   identical
   islinspace
   issorted

`inf` and `nan` handling
~~~~~~~~~~~~~~~~~~~~~~~~

Special-value (`inf` and `nan`) checks.
`inf` and `nan` in the *variances is ignored*.

.. autosummary::
   :toctree: ../generated

   isnan
   isinf
   isfinite
   isposinf
   isneginf
   nan_to_num

Reduction
~~~~~~~~~

Reduction operations are operations to remove one or more dimension, e.g., by performing a sum over all elements along a dimension.

.. autosummary::
   :toctree: ../generated

   all
   any
   cumsum
   max
   mean
   min
   nanmax
   nanmean
   nanmin
   nansum
   sum

Trigonometric
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   sin
   cos
   tan
   asin
   acos
   atan
   atan2

Geometric
~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   geometry.position
   geometry.x
   geometry.y
   geometry.z

Group-by (split-apply-combine)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Reduction
---------

.. autosummary::
   :toctree: ../generated

   groupby
   GroupByDataArray.all
   GroupByDataArray.any
   GroupByDataArray.max
   GroupByDataArray.mean
   GroupByDataArray.min
   GroupByDataArray.sum
   GroupByDataset.all
   GroupByDataset.any
   GroupByDataset.max
   GroupByDataset.mean
   GroupByDataset.min
   GroupByDataset.sum

Other
-----

.. autosummary::
   :toctree: ../generated

   GroupByDataArray.copy
   GroupByDataset.copy

Counts
~~~~~~

"Counts" refers to the default unit of data after histogramming.
It is also known as frequency.
This can be converted into a counts (frequency) density, e.g., for visualization purposes.

.. autosummary::
   :toctree: ../generated

   counts_to_density
   density_to_counts

Visualization
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   show
   table
   to_html
   plot


Compatibility
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   to_dict
   from_dict
