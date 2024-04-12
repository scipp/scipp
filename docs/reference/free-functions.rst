.. currentmodule:: scipp

Free functions
==============

General
~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   bin
   bins
   bins_like
   collapse
   curve_fit
   elemwise_func
   group
   hist
   logical_not
   logical_and
   logical_or
   logical_xor
   lookup
   nanhist
   merge
   midpoints
   rebin
   reduce
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
   erf
   erfc
   exp
   floor
   floor_divide
   log
   log10
   mod
   multiply
   negative
   norm
   pow
   reciprocal
   round
   sqrt
   subtract

Shape
~~~~~

.. autosummary::
   :toctree: ../generated/functions

   broadcast
   concat
   flatten
   fold
   squeeze
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

``inf`` and ``nan`` handling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Special-value (``inf`` and ``nan``) checks.
``inf`` and ``nan`` in the *variances is ignored*.

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
   median
   min
   nanmax
   nanmean
   nanmedian
   nanmin
   nanstd
   nansum
   nanvar
   std
   sum
   var

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

Hyperbolic
~~~~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   sinh
   cosh
   tanh
   asinh
   acosh
   atanh

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

   make_html
   make_svg
   plot
   show
   show_graph
   table
   to_html


Logging
~~~~~~~

.. autosummary::
   :toctree: ../generated/functions

   get_logger
   display_logs
