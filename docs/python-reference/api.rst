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
   all
   any
   concatenate
   dot
   filter
   histogram
   max
   mean
   merge
   min
   nan_to_num
   norm
   rebin
   reciprocal
   reshape
   sort
   sqrt
   sum

Group-by (split-apply-combine)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

Comparison
~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   less
   greater
   less_equal
   greater_equal
   equal
   not_equal

Counts
~~~~~~

"Counts" refers to the default unit of data after histogramming.
It is also known as frequency.
This can be converted into a counts (frequency) density, e.g., for visualization purposes.

.. autosummary::
   :toctree: ../generated

   counts_to_density
   density_to_counts
