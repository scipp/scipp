.. _deployment:

Deployment
==========

The scipp project encompases a collection of packages in an ecosystem.
This documents the release/deployment process for each package.

Mantid Framework
-----------------

Mantid is an optional runtime dependency for scipp and is utilised solely through its python api.
In practice it is an important component for using scipp for neutron scattering as it provides a number of key components, such as facility specific file loading.

For mac os and linux, ``mantid-framework`` conda packages can be produced and are placed in the anaconda ``scipp`` channel.
There is currently no windows package.

*Key Locations*

* Source code for `mantid <https://github.com/mantidproject/mantid>`_.
* Code for the `recipe <https://github.com/scipp/mantid_framework_conda_recipe>`_.
* CI `Azure pipelines <https://dev.azure.com/scipp/mantid-framework-conda-recipe/_build>`_.
* Publish location on `Anaconda Cloud <https://anaconda.org/scipp/mantid-framework>`_.

We have two azure piplelines for these packages.

Our first pipeline will build, test and publish a package to the ``scipp`` channel and is triggered by new tags in the `recipe repository <https://github.com/scipp/mantid_framework_conda_recipe>`_.
Successful pipline execution pushes new packages to `Anaconda Cloud <https://anaconda.org/scipp/mantid-framework>`_.
This is the release pipeline, and is the subject of the deployment procedure below.

Our second pipleline uses latest master of mantid to produce (but not publish) a nightly package.
This allows us to anticipate and correct problems we will encounter in new package generation, and ensures we can produce new packages at short notice against an evolving mantid target, while taking into account updated depenencies on conda.

Mantid Framework Deployment Procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#. Check the nightly build pipeline on azure and verify successful completion.
   If they are not, investigate and fix problems there first.
#. Determine the git revision of mantid you wish to use.
#. Compute a version string for the revision using ``tools/make_version.py`` from the recipe repository.
#. From the recipe repository update ``.azure-pipelines/release.yml`` setting the ``git_rev`` and ``mantid_ver`` to match the two values from the previous steps.
#. Commit changes to recipe repository to master (peer review advisable).
#. Create a new annotated tag in the recipe repository to describe the release and its purppose 
#. Push the tag to origin, which will trigger the tagged release pipeline

Mantid Framework Verification Procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If the previous steps ran correctly you will have new packages on `Anaconda Cloud <https://anaconda.org/scipp/mantid-framework>`_. 
It is advisable to install the new ``mantid-framework`` and ``scipp`` together and then to run the compatibility tests for mantid.py in the scipp source code to ensure the interfaces work as expected.

.. note::
  The process outlined here will be used as part of the CI in future.

.. code-block:: sh 

  $ conda create \
      -n env_test_scipp_and_mantid \
      -c conda-forge \
      -c scipp \
      python=3.7 \
      scipp \
      mantid-framework \
      pytest \
      psutil \

  $ conda activate env_test_scipp_and_mantid

  $ python -m pytest python/tests/compat/test_mantid.py  
