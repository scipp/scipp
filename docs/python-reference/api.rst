.. currentmodule:: scipp

Classes
=======

.. autosummary::
   :toctree: ../generated

   Dim
   Unit
   Variable
   VariableView
   DataArray
   DataArrayView
   Dataset
   DatasetView
   GroupByDataArray
   GroupByDataset

Free functions
==============

See also :ref:`scipp-neutron` for functions specific to handling neutron-scattering data.

General
~~~~~~~

.. autosummary::
   :toctree: ../generated

   abs
   choose
   collapse
   concatenate
   dot
   exp
   filter
   histogram
   is_sorted
   log
   log10
   merge
   nan_to_num
   norm
   rebin
   reciprocal
   reshape
   slices
   sort
   sqrt
   transpose
   values
   variances

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
   is_approx
   is_equal
   is_linspace
   is_sorted

Reduction
~~~~~~~~~

Reduction operations are operations to remove one or more dimension, e.g., by performing a sum over all elements along a dimension.

.. autosummary::
   :toctree: ../generated

   all
   any
   max
   mean
   min
   nanmax
   nanmin
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
   GroupByDataArray.flatten
   GroupByDataArray.max
   GroupByDataArray.mean
   GroupByDataArray.min
   GroupByDataArray.sum
   GroupByDataset.all
   GroupByDataset.any
   GroupByDataset.flatten
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
   plot.plot


Compatibility
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   to_dict
   from_dict
