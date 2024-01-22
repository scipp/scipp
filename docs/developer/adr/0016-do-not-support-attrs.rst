ADR 0016: Do not support "attrs"
================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon, Sunyoung
- Date: 2023-06-06

Context
-------

Background
~~~~~~~~~~

Arbitrary user-defined attributes appear in many applications.
Examples include:

- HDF5 attrs for datasets and groups
- Attributes in NetCDF
- Attributes in Zarr
- Attributes in NeXus (typically using HDF5 attrs)
- ``xarray.Variable.attrs`` and ``xarray.DataArray.attrs``

Scipp's ``DataArray`` supports ``attrs``.
Note that currently ``attrs`` doubles as a place to store "unaligned" coords, which is not directly related to the topic discussed here.
However, ``scipp.Variable`` does not support ``attrs``.
In part this is due to a reduced need for ``attrs``, since Scipp has native support for physical units, which is one of the recurring applications of ``attrs`` in other libraries.
The other reason is legacy:
``scipp.Variable`` was written in C++ and was intended to be lightweight and simple.

When interacting with files such as HDF5 or NeXus we encounter a recurring need to load arbitrary attributes.
Currently this is not possible in Scipp, at least not if we want to represent an HDF5 dataset as a ``scipp.Variable``.
Support for ``scipp.Variable.attrs`` would allow for representing arbitrary attributes in a ``scipp.Variable``.
The question is whether this is a good idea.

Analysis
~~~~~~~~

Addition of ``scipp.Variable.attrs`` would add a significant amount complexity to the code base.
It would likely also impact performance in some applications, as we would need to handle references to Python objects (requiring GIL acquisition in C++ code) or shared pointers.
It could furthermore increase the chance of running into problems with Python garbage collection similar to the existing (low-probability) issues described in https://github.com/scipp/scipp/issues/2813.

Aside from these technical hurdles (which could in principle be overcome or partially ignored) we need to consider the conceptual aspects.
Is a dictionary of arbitrary attributes a good solution for a real problem?
Obviously, attributes of datasets in files are widespread and have valid and important applications.
However, Scipp's main purpose is *processing* data, not *storing* data.
The attributes encountered in files strictly describe the array or dataset as it was stored, and are not necessarily valid after modifying it.

If we consider binary operations (such as addition or division) then it is not clear how attributes should be handled if they mismatch between operands or are missing in one operand.
This is an example where Scipp made a different choice than Xarray.
Consider for example three (Scipp or Xarray) data arrays, ``a``, ``b``, and ``c``, each with an attribute named ``attr1``, each with a different value.
In Xarray, ``((a + b) + c).attrs != (a + (b + c)).attrs`` (on the left, ``attr1`` is preserved with the value it had in ``c`` whereas on the right the value in ``a`` is preserved) whereas in Scipp ``((a + b) + c).attrs == (a + (b + c)).attrs`` (``attr1`` is dropped).
Scipp ensures this by treating a missing attribute in one operand as mismatches and thus drops the attribute.
This ensures consistency, but comes at a significant cost:
In practice attributes are very frequently dropped in binary operations.
This raises some questions about the usability of the mechanism.
Xarray's approach is more "useful", at the cost of consistency.
Xarray's mechanism furthermore necessitates complications, such as global settings that influence the behavior as well as certain interfaces with keyword arguments to define attribute handling.

Ignoring binary operations, one could argue that keeping attributes is still useful for unary operations where those problems do not arise.
However, attributes can nevertheless become invalid:
For example, a "long_name" attribute may no longer be valid after a unit conversion, or after applying a unary function such as ``np.power`` or ``np.sin``.
The same holds for a "units" attribute &mdash; an issue Scipp avoids by having native handling of units.
Here is a brief code example illustrating how this goes badly wrong in Xarray:

.. code-block:: python

    import numpy as np
    import xarray as xr

    da = xr.DataArray(np.arange(10), coords=(('x', np.arange(10)),))
    da.coords['x'].attrs['units'] = 'm'
    da.coords['x'].attrs['long_name'] = 'Distance'
    da.attrs['units'] = 'K'
    da.attrs['long_name'] = 'Temperature'

    np.power(da, 2).plot()

The resulting plot is labelled with "Temperature [K]" on the y-axis, which is clearly wrong and dangerously misleading.

We see this as an indicator that any attempt to handle "unknown" attributes is bound to fail.
While we acknowledge that there are valid and important use cases for attributes, we believe that the risk of introducing invalid attributes in operations applied to variables or data arrays is too high and outweighs the benefits of supporting attributes.

Proposed solution
~~~~~~~~~~~~~~~~~

- Do not add support for ``scipp.Variable.attrs``.
- Remove support for ``scipp.DataArray.attrs``.
  The current double-use for unaligned-coords will be supported via an alignment flag accessed via ``scipp.DataArray.coords``.

Alternative solutions
~~~~~~~~~~~~~~~~~~~~~

We could consider support for *storing* attributes, but *dropping* them on any modification of the data.
An HDF5 dataset can then be loaded including its attributes, while at the same time avoiding the risk of invalidating attributes in operations.
However, unless we lock down mutable access to ``scipp.Variable.values`` such a mechanism would always have loopholes.
Is there a viable and safe mechanism for such attributes?
Is there a sufficient number of applications where such a restricted attribute support would be worthwhile?

Decision
--------

- Clearly document the scope of Scipp, in particular that data structures must remain conceptually simple.
- Go with proposed solution, i.e., remove ``attrs`` from ``DataArray``.

Consequences
------------

Positive:
~~~~~~~~~

- Simpler code.
- Cleaner semantics.
  Scipp data structures will be a simple and basic building block for labelled data.
- Avoids problems with potentially invalid attributes in results of operations.

Negative:
~~~~~~~~~

- Removing ``scipp.DataArray.attrs`` represents a breaking change that will affects users.
  We need to find and implement alternative solutions, e.g., by handling attributes on a higher level.
- Fully representing information from HDF5 or similar files will not be possible directly.
