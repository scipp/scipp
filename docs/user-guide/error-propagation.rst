.. _propagation_of_uncertainties:

Propagation of uncertainties
============================

If present, scipp propagates uncertainties (errors) as described in `Wikipedia: Propagation of uncertainty <https://en.wikipedia.org/wiki/Propagation_of_uncertainty>`_.
The implemented mechanism assumes *uncorrelated data*.

An overview for key operations is:

.. table::
    :widths: 50 50

    +-------------------+-------------------------------------------------------------------------+
    |Operation :math:`f`|Variance :math:`\sigma^{2}_{f}`                                          |
    +===================+=========================================================================+
    |:math:`-a`         |:math:`\sigma^{2}_{a}`                                                   |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`|a|`        |:math:`\sigma^{2}_{a}`                                                   |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`\sqrt{a}`   |:math:`\frac{1}{4} \frac{\sigma^{2}_{a}}{a}`                             |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`a + b`      |:math:`\sigma^{2}_{a} + \sigma^{2}_{b}`                                  |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`a - b`      |:math:`\sigma^{2}_{a} + \sigma^{2}_{b}`                                  |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`a * b`      |:math:`\sigma^{2}_{a}b^{2} + \sigma^{2}_{b}a^{2}`                        |
    +-------------------+-------------------------------------------------------------------------+
    |:math:`a / b`      |:math:`\frac{\sigma^{2}_{a} + \sigma^{2}_{b} \frac{a^{2}}{b^{2}}}{b^{2}}`|
    +-------------------+-------------------------------------------------------------------------+

The expression for division is derived from :math:`(\frac{\sigma_{a/b}}{a/b})^{2} = (\frac{\sigma_{a}}{a})^{2} + (\frac{\sigma_{b}}{b})^{2}`.
