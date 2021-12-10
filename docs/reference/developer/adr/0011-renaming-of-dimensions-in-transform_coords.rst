ADR 0011: Renaming of Dimensions in ``transform_coords``
========================================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2021-12-10

Context
-------

:py:func:`scipp.transform_coords` computes new coordinates from other coordinates and attributes.
This is often used to express conversion of dimension-coordinates to a single output such as ``datetime`` to ``local_time`` in the user guide or ``tof`` to ``wavelength`` in scippneutron.
As those represent changes of coordinate systems, it makes sense to change the names of dimensions as well.

Since :py:func:`scipp.transform_coords` is a general purpose function, it cannot make assumptions about the meaning of coordinates.
Any algorithm that determines how dimensions are renamed must therefore be based solely on the input data (``DataArray`` / ``Dataset`` and ``graph``).
It must furthermore not depend on the order the graph is traversed in, as that order is an implementation detail.
When there is more than one possible choice how dimensions can be renamed, it is better to not rename at all and leave it up to the user's judgment.

Constraints
~~~~~~~~~~~

Commutability with slicing and concatenating
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It would be desirable to handle dimension renaming in such a way that slicing the data in a dimension, transforming coordinates of each slice, and finally concatenating the slices produces the same result as transforming the original coordinates.
This would allow distributing slices over multiple nodes or performing specialized computations on each slice.
It is not possible, however.

Consider the following graph.
In this notation, ``x[x,y]`` is a coordinate with name ``x`` and dimensions ``[x,y]``.
In the first case (left), the coordinate transformation is performed on the original coordinates.
The resulting ``c`` has dimensions ``[a,x,b]`` because it depends on two dimension-coordinates and we cannot automatically determine a single dimension to rename.
On the right, the input data is first sliced in dimension ``[a]`` and ``c`` is computed on every slice.
Here, ``b`` is the only dimension-coordinate in the inputs and dimension ``[b]`` is therefore renamed to ``[c]``.
When we finally concatenate the transformed slices, ``c`` has dimensions ``[a,x,c]`` which differ from the result in the left graph.

.. image:: ../../../images/transform_coords/slice-transform-concat.svg
  :width: 640
  :alt: slice-transform-concat


Decision
--------




Consequences
------------

Positive:
~~~~~~~~~


Negative:
~~~~~~~~~

