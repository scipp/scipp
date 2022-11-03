ADR 0014: Switch to Calendar Versioning
=======================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2022-11-03

Context
-------

Scipp is currently using semantic versioning.
As we have not made a "1.0" release yet, i.e., we have 0-based version numbers, the concrete implementation of semantic versioning is actually unclear.
A very thorough analysis and explanation of the shortcomings of semantic versioning can be found in `Semantic Versioning Will Not Save You <https://hynek.me/articles/semver-will-not-save-you/`_ and `Should You Use Upper Bound Version Constraints? <https://iscinumpy.dev/post/bound-version-constraints/>`_.
We will not repeat the arguments here, but it is essential to read these references for understanding the decision.

An alternative that seems be gaining popularity is `Calendar Versioning <https://calver.org/>`_.
`Designing a version <https://sedimental.org/designing_a_version.html>`_ provides further insights and considerations.

Decision
--------

- Switch to calendar versioning in the YYYY-0M-MICRO numbering scheme.
- Adopt and document an explicit deprecation strategy.

Consequences
------------

Positive:
~~~~~~~~~

- Avoids the problems with calendar versioning described in detail in the linked articles.
- Users (including downstream libraries) can see immediately see how old the version they are using is.
- Avoid tempting users to pin to, e.g., major versions.

Negative:
~~~~~~~~~

- Future major version numbers that Scipp would have had can no longer be used to guess whether major breaking changes occured.
