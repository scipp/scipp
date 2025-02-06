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

2. Commit and merge the changes on GitHub.

3. Create a release in GitHub.
   This triggers workflows to create conda packages and wheels.
   They are automatically uploaded to conda-forge and PyPI, respectively.
   This will also publish the new documentation.
   If the major or minor release have been incremented this will furthermore rebuild the docs of the previous major or minor release with a banner indicating that the release is outdated.

Updating the logo
-----------------

- Edit the logo in ``resources/``
- Copy the logo into ``docs/_static/``.
  Convert the font to a path.
  This ensures that we get the correct font on all machines.
  Make sure the page is resized to match the drawing contents since the SVG is included directly in the documentation HTML.
- Export the logo *without text* from the SVG as PNG.
- Create the favicon using ``convert icon.png -define icon:auto-resize="128,96,64,48,32,16" favicon.ico``
- Update ``docs/conf.py`` if filenames have changed.

Updating an expired Anaconda token
----------------------------------

Tokens for uploading conda packages to the `Anaconda website <https://anaconda.org/scipp>`_ have a limited lifetime.
When a token expires, a new one has to be created and added to the Github `Scipp organisation <https://github.com/scipp>`_, following these steps:

- Go to https://anaconda.org/scipp/settings/access (requires admin access privileges)
- Give the token a name (e.g. ``github-actions-2022``)
- Select ``Strength=Strong``
- Scope: select ``Allow read access to the API site`` and ``Allow write access to the API site``
- Choose an expiry date (the token will be valid for 1 year by default)
- Click ``Create``
- Copy the token hash displayed on the webpage
- Go to Scipp's `Github organisation <https://github.com/scipp>`_
- Go to ``Settings > Security > Secrets > Actions``
- Delete the old (expired) ``ANACONDATOKEN``
- Click the ``New organization secret`` button
- Name it ``ANACONDATOKEN`` and paste the token hash in the ``Value`` field
- Select the repositories that should have access to the token: namely ``scipp``, ``scippneutron``, ``scippnexus``, ``ess``, ``plopp``
- Click ``Add secret``

Nightly releases
----------------

We also maintain nightly releases which are uploaded to a `custom PyPI index <https://pypi.anaconda.org/scipp-nightly-wheels/simple/>`_.
To consume these packages, users need to set ``index-url`` in their ``pip`` configuration.

.. code-block:: bash

    python -m pip install \
      --pre \
      --index-url https://pypi.anaconda.org/scipp-nightly-wheels/simple/ \
      --extra-index-url https://pypi.org/simple \
      scipp

The order of the index urls matter and if you are using ``uv`` users need to set the ``extra-index-url`` in the configuration.

.. code-block:: bash

    python -m uv pip install \
    --pre \
    --extra-index-url https://pypi.anaconda.org/scipp-nightly-wheels/simple/ \
    scipp
