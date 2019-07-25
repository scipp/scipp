.. _units:

Physical units
==============

All variables in scipp have a physical unit.
Variables are used for coordinates, labels, data, as well as attributes, i.e., all of these have a unit.

When not specified explicitly the unit of a variable defaults to :py:class:`scipp.units.dimensionless`, i.e., the variable is considered dimensionless.

:py:class:`scipp.units` provides a number of pre-defined elementary units as well as operations between units.
This can be used to create units that do not have a pref-defined identifier:

.. ipython:: python

    import scipp as sc

    length = sc.units.m
    area = length * length
    area
    volume = length * length * length
    volume
    speed = length / sc.units.s
    speed

Due to a restriction in scipp's units implementation the set of supported units is unfortunately limited a compile-time of the underlying C++ library:

.. ipython:: python
    :okexcept:

    sc.units.s / volume

See :ref:`customizing` on how additional units can be added.
