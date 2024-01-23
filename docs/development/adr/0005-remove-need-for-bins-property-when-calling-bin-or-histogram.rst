ADR 0005: Remove need for ``bins`` property when calling ``bin`` or ``histogram``
=================================================================================

- Status: accepted
- Deciders: Jan-Lukas, Matthew A., Neil, Owen, Simon
- Date: 2021-01-14

Context
-------

To apply operations to bins in binned data we generally use the ``.bins`` property, which returns a helper object.
For example, ``data.bins.sum()`` applies a sum along the bin dimension to each bin, and returns a new variable containing the sums as elements.
For consistency this is also required in, e.g. ``sc.histogram(data.bins, ...)`` and ``sc.bin(data.bins, ...)`` when histogramming or binning data with prior binning.

However, users will mostly apply this functions to data with prior binning, i.e., the ``.bins`` property has to be used almost always.
This also requires teaching this concept to more users.

Since we do not allow nested binning or histogramming, it is in practice always clear what these operations should do, even without the ``.bins`` property, i.e., it is unnecessary noise and complication.

Decision
--------

Remove need for ``.bins`` when calling ``sc.histogram`` or ``sc.bin`` on binned data.

Consequences
------------

Positive:
~~~~~~~~~

- Shorter and easier syntax for most users.
- Generic client code that can handle binned as well as un-binned data becomes easier to write.

Negative:
~~~~~~~~~

- ``sc.bin`` and ``sc.histogram`` will strictly speaking handle data differently, depending on whether they are called with binned or histogrammed data, i.e., their definition is made slightly more fuzzy.
