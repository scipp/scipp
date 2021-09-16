.. currentmodule:: scipp

Classes
=======

.. autosummary::
   :toctree: ../generated/classes
   :template: scipp-class-template.rst
   :recursive:

   Bins
   DataArray
   Dataset
   GroupByDataArray
   GroupByDataset
   Unit
   Variable
   typing.VariableLike
   typing.MetaDataMap

Creation functions
==================

.. autosummary::
   :toctree: ../generated/functions

   array
   arange
   empty
   empty_like
   full
   full_like
   geomspace
   linspace
   logspace
   matrix
   matrices
   ones
   ones_like
   scalar
   vector
   vectors
   zeros
   zeros_like

Free functions
==============

General
~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   bin
   bins
   choose
   collapse
   histogram
   logical_and
   logical_or
   logical_xor
   merge
   rebin
   slices
   sort
   stddevs
   to_unit
   transform_coords
   values
   variances
   where

Math
~~~~

.. autosummary::
   :toctree: ../generated/functions

   abs
   add
   ceil
   cross
   divide
   dot
   exp
   floor
   floor_divide
   log
   log10
   mod
   multiply
   norm
   reciprocal
   round
   sqrt
   subtract

Shape
~~~~~

.. autosummary::
   :toctree: ../generated/functions

   broadcast
   concatenate
   flatten
   fold
   transpose

Comparison
~~~~~~~~~~

Comparison operators compare element-wise and *ignore variances*.

.. autosummary::
   :toctree: ../generated/functions

   allclose
   allsorted
   equal
   greater
   greater_equal
   identical
   isclose
   islinspace
   issorted
   less
   less_equal
   not_equal

`inf` and `nan` handling
~~~~~~~~~~~~~~~~~~~~~~~~

Special-value (`inf` and `nan`) checks.
`inf` and `nan` in the *variances is ignored*.

.. autosummary::
   :toctree: ../generated/functions

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
   :toctree: ../generated/functions

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
   :toctree: ../generated/functions

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
   :toctree: ../generated/functions

   geometry.position
   geometry.rotation_matrix_from_quaternion_coeffs

Group-by (split-apply-combine)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Reduction
---------

.. autosummary::
   :toctree: ../generated/functions

   groupby

Counts
~~~~~~

"Counts" refers to the default unit of data after histogramming.
It is also known as frequency.
This can be converted into a counts (frequency) density, e.g., for visualization purposes.

.. autosummary::
   :toctree: ../generated/functions

   counts_to_density
   density_to_counts

Visualization
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   show
   show_graph
   table
   to_html
   plot


Compatibility
~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   to_dict
   from_dict
   compat.from_pandas
   compat.from_xarray

Modules
=======

.. autosummary::
   :toctree: ../generated/modules
   :template: scipp-module-template.rst
   :recursive:

   constants
   spatial

