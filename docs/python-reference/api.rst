.. currentmodule:: scipp

Classes
=======

.. autosummary::
   :toctree: ../generated

   Dim
   Unit
   Variable
   VariableProxy
   DataArray
   DataProxy
   Dataset
   DatasetProxy
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
   concatenate
   dot
   filter
   histogram
   mean
   merge
   norm
   rebin
   reshape
   sort
   sqrt
   sum

Group-by (split-apply-combine)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   groupby
   GroupByDataArray.flatten
   GroupByDataArray.mean
   GroupByDataArray.sum
   GroupByDataset.flatten
   GroupByDataset.mean
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
   
Counts
~~~~~~

"Counts" refers to the default unit of data after histogramming.
It is also known as frequency.
This can be converted into a counts (frequency) density, e.g., for visualization purposes.

.. autosummary::
   :toctree: ../generated

   counts_to_density
   density_to_counts

Data types
==========

.. autosummary::
   :toctree: ../generated

   dtype
