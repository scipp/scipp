ADR 0014: Switch to Calendar Versioning
=======================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2022-11-03

Context
-------

Scipp is currently using semantic versioning (SemVer).
As we have not made a "1.0" release yet, i.e., we have 0-based version numbers, the concrete implementation of SemVer is actually unclear.
A very thorough analysis and explanation of the shortcomings of SemVer can be found in `Semantic Versioning Will Not Save You <https://hynek.me/articles/semver-will-not-save-you/>`_ and `Should You Use Upper Bound Version Constraints? <https://iscinumpy.dev/post/bound-version-constraints/>`_.
We will not repeat the arguments here, but it is essential to read these references for understanding the decision.
While the above references do not argue *against* SemVer, they highlight a variety of shortcomings and show that the perceived benefits of SemVer frequently do not work out in practice.

An alternative that seems be gaining popularity is `Calendar Versioning <https://calver.org/>`_ (CalVer).
`Designing a version <https://sedimental.org/designing_a_version.html>`_ provides further insights and considerations.

We frequently encounter the need to make patch-releases and therefore we *do* see the value in keeping a micro or patch part in the version number.
This is not in contradiction to CalVer since, e.g., year/month information can be combined with a patch number to form a version number.

Regarding the major and minor version numbers, we have two observations:

Our version numbers are currently 0-based.
Initially this was "because Scipp is not ready for (production) use".
However, it is unclear when we should move away from this.
The consequence is that every release we make is kind of breaking.
To derive value from SemVer, we should thus make a "1.0" release.

Then, as Scipp is a *library*, there might be the tendency to never make huge breaking changes, i.e., reluctance to increase the major version.
We might thus converge to a case like NumPy, which effectively has a 1.MINOR.PATCH versioning scheme, i.e., the major version is "always" 1 --- which is what SemVer is about, so this is a good thing in the case of a very stable library such as NumPy.
However, reluctance to increase the major version (similar to the reluctance to jump from 0.X to 1.0) is related to a number of potential problems:

- We may decide against big (but necessary) changes (and thus a major version bump), as it brings the risk of users continuing to use the previous major release.
  We do not have the capacity to support this, e.g., by backporting bugfixes to a previous (or multiple previous) major versions.
- We may be tempted to "downplay" how big changes are, to avoid bumping the major version, to avoid rapidly increasing major versions (which would be a problem for users relying on SemVer major version to not change too frequently).
  Thus, minor version increases may be more breaking than good and we lose the benefits of SemVer, as users will suddenly have to treat major *and* minor version changes as potentially breaking.
- Following a "stricter" SemVer policy, i.e., bumping the major version also for relatively small changes, implies that users will not be able to tell whether a major release is literally major, or contains just some minor changes irrelevant to them.
  SemVer would thus provide more information about details of code instead of higher-level semantics, which is what users may care about most.

Decision
--------

- Switch to CalVer in the YY-0M-MICRO numbering scheme.
- Adopt and document an explicit deprecation strategy.

Consequences
------------

Positive:
~~~~~~~~~

- Avoids the problems with SemVer described in detail in the linked articles, mainly by forcing us to use a clearer deprecation policy.
- Users (including downstream libraries) can see immediately see how old the version they are using is.
- Avoid tempting users to pin to, e.g., major versions.

Negative:
~~~~~~~~~

- Future major version numbers that Scipp would have had can no longer be used to guess whether major breaking changes occurred.
- Reversing the decision will be difficult, as it would lead to odd major version numbers that look like years but are not.
