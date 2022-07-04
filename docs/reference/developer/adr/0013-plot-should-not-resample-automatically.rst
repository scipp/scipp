ADR 0013: Plot should not resample automatically
================================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2022-06-22

Context
-------

The current plotting functionality automatically resamples 2-D (or higher) data as well as binned data of any dimension.
There were three main drivers/motivations for this:

- We frequently deal with data with "long" dimensions, such as "pixel" or "spectrum" dimensions, which may exceed a million.
- We often deal with "ragged" data, i.e., data with 2-D coordinates that originate after coordinate transformations.
- We frequently plot binned data.
  Bins are often much larger than the target plot resolution, so histogramming must be done.
  This is combined with "long" dimensions and potentially high resolutions along other dimensions (exceeding 1000 or even 10000).

In the past we have therefore taken it as a given that resampling is a non-optional requirement.
However, we have since then experienced a multitude of problems with this, ranging from simple bugs to complex and hard to maintain code.
More importantly, the resulting plots are unpredictable and not faithful to the actual data.
Concrete problems are:

- Infinite zoom with event data is not useful and led to considerations on a different (more complicated) mechanism, such as stopping the resampling at a specific level or providing additional user-control over the resampling steps.
- Log-scale plots should use log bins to be truly useful, but is appears to be non-trivial to implement.
- Picking values for the profile view has unpredictable and misleading values depending on zoom level.
  This has been improved in the recent refactor by growing the bounds of the selection to the underlying data bins, but the solution is incomplete since it does not resolve the problem when multiple data bins contribute to a display pixel.
- Complex code related to resampling throughout the plotting code.
  While the actual resampling bit is encapsulated in ``ResamplingModel`` there is a significant number of complications ranging from custom handling of bounds on zoom event to handling of coord/axis tic labels.
- Misleading display of data values, which are integrated over display pixel rather than data pixel (as a consequence, the values change with zoom level).
- Resampling may be done using a "sum" or "mean" mode.
  In many cases this can be determined from the physical unit of the data, and the implementation "guessed" the correct mode on this.
  This is however not foolproof and the behavior was visible to the user only in GUI elements, i.e., not in static or saved figures.
- Complicated handling of datetimes that also affects resampling.
- Glitches and extremely small bin values from barely overlapping data and display pixels lead to unreadable plots with extremely large color scales.
- Scientifically any plot generated with resampling is not usable, e.g., for publications.

Related topics
---------------

The resampling behavior is part of a wider effort for a simpler and more flexible plotting code.
This includes more flexible plotting backends and a pipeline mechanism for building custom interactive visualizations.
These are not the topic of this ADR.

Decision
--------

- Remove automatic resampling and binning from the plot function.

Consequences
------------

Positive:
~~~~~~~~~

- Faithful representation of data.
- Simpler and more maintainable code.

Negative:
~~~~~~~~~

- Breaking change to current behavior will temporarily affect users.
- User has to be more explicit, in particular when plotting binned data.
  A new and convenient API for this is under development.
- Plots with very long dimensions cannot be created directly.
  The user needs to resample themselves.
  We see this requirement mainly as an artifact from users used to old software and its inflexibilities, as well as applications from times when neutron scattering instruments had very few pixels.
  With modern highly pixelated instruments these plots likely have very limited use.

