ADR 0015: Disable support for broadcasting variances
====================================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2023-01-10

Context
-------

One of Scipp's key features is propagation of uncertainties in operations.
The mechanism was carried over from an equivalent mechanism in Mantid.
Our recent publication `Systematic underestimation of uncertainties by widespread neutron-scattering data-reduction software <https://doi.org/10.3233/JNR-220049>`_ highlights a ubiquitous problem in this mechanism.

In neutron-scattering applications normalization terms typically have lower dimensionality than the data and are thus broadcast in operations.
This leads to unhandled correlations in the normalized result.
The effect is that uncertainties of normalization terms are strongly suppressed and effectively ignored.
However, the user is not aware of this as the mechanism that Scipp provides seems to promise correct handling.
This can lead to erroneous attribution of statistical significance in final results that scientists may publish.

We have considered issuing a warning instead of raising an exception.
However, past experience shows that warnings tend to get ignored.
Given the severity of the issue we thus do not consider this a solution.

Decision
--------

Raise an exception when implicitly or explicitly broadcasting variable elements that have variances.

Consequences
------------

Positive:
~~~~~~~~~

- Avoids hidden underestimation of uncertainties.
- Avoid futile burning of CPU resources.
- May help to push scientists to adopt improved approaches for handling uncertainties.

Negative:
~~~~~~~~~

- User action is required, as this will break most workflows.
