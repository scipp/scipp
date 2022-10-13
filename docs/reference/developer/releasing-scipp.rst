Releasing Scipp
===============

Purpose
-------

This document describes steps to be taken to prepare and make a new Scipp release, or perform related changes

Release steps
-------------

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

Updating the logo
-----------------

- Edit the logo in ``ressources/``
- Copy the logo into ``docs/_static/``.
  Convert the font to a path.
  This ensures that we get the correct font on all machines.
  Make sure the page is resized to match the drawing contents since the SVG is included directly in the documentation HTML.
- Export the logo *without text* from the SVG as PNG.
- Create the favicon using ``convert icon.png -define icon:auto-resize="128,96,64,48,32,16" favicon.ico``
- Update ``docs/conf.py`` if filenames have changed.
