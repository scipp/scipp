ADR 0007: Do not support arguments referring to ``x`` or ``y``
==============================================================

- Status: proposed
- Deciders: Jan-Lukas, Neil, Owen, Simon
- Date: 2021-05-05

Context
-------

Particularly in Scipp's plotting functionality there can be confusion from different interpretation of ``x`` or ``y``:

- Dimensions ``'x'`` or ``'y'`` of the data being handled (plotted).
- ``x`` and ``y`` axes of a (matplotlib) figure, e.g., as referenced by the ``xlim`` and ``ylim`` args matplotlib provides.
- The meaning of above ``x`` and ``y`` and the configured limits after the "transpose" button in the plot was activated.

Furthermore it should be considered that Scipp supports dimensions in arbitrary order.
That is, data depending on ``dim0`` and ``dim1`` may be stored as ``[dim0, dim1]`` or as ``[dim1, dim0]``.
Creating a plot from this with hypothetical ``xlim`` or ``xmin`` arguments to ``plot()`` would thus yield different and unpredictable results depending on an, in principle, irrelevant detail of the data.

The ambiguity can be avoided using Scipp's ubiquitous approach of requiring explicit dimension labels.
One example for this is the ``scale`` argument of ``plot()``, which is a dictionary mapping from dimension labels to the desired scale (linear or logarithmic).
The same approach should be adopted for all other arguments to ``plot()``.

Note that in the case of defining limits, an alternative approach recommended to users is the use of slicing.

Decision
--------

Do not provide or support arguments referring to ``x`` or ``y``.

Consequences
------------

Positive:
~~~~~~~~~

- Self documenting syntax.
- Avoid unpredictable resulting plots if dimension order changes.

Negative:
~~~~~~~~~

- Not supporting ``xlim`` or other arguments with names identical to matplotlib syntax increases the learning effort.
- Marginally more typing.
