Releasing Scipp
===============

Purpose
-------

This document describes steps to be taken to prepare and make a new scipp release

Steps
-----

1. Update ``docs/about/whats-new.ipynb`` as required to describe highlights and key changes of the release.
   When doing this, also consider removing information about past releases.
   We typically keep information on the "What's New" page for approximately two to four past releases.
   The concrete duration is decided case-by-case, based on the relevance of a particular topic, e.g., how critical it is that users do not miss the change, and based on the frequency of releases.

2. Update ``docs/about/release-notes.rst`` to mention the release month.

3. Commit and merge the changes on GitHub.

4. Create a release in GitHub.
   This triggers workflows to create conda packages and wheels.
   They are automatically uploaded to conda-forge and PyPI, respectively.
   This will also publish the new documentation.
   If the major or minor release have been incremented this will furthermore rebuild the docs of the previous major or minor release with a banner indicating that the release is outdated.
