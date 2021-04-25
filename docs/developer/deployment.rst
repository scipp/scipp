.. _deployment:

Deployment
==========

The scipp project encompases a collection of packages in an ecosystem.
This documents the release/deployment process for each package.

Mantid Framework
----------------

Mantid is an optional runtime dependency for scippneutron and is utilised solely through its python api.
In practice it is an important component for using scippneutron for neutron scattering as it provides a number of key components, such as facility specific file loading.

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

Our second pipleline uses latest ``main`` of mantid to produce (but not publish) a nightly package.
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

.. note::
  As part of the ``conda build`` step mantid's imports are tested and the mantid-scipp interface is tested in ``run_test.sh`` `(see conda docs) <https://docs.conda.io/projects/conda-build/en/latest/resources/define-metadata.html#run-test-script>`_. Packaging can therefore fail if mantid does not appear to work (import) or there are incompatibilities between scipp and mantid. The compatibility checks take the test code from the latest scippneutron. scippneutron itself is installed from the ``scipp/label/dev`` (dev) channel as part of the process. All this means that the package can fail generation at the final test stage (all within ``conda build``) despite ``mantid-framework`` itself building and packaging,  so check reasons for packaging failure by inspecting full log output on azure pipeline.
  
.. warning::
  When running ``conda build`` locally, ensure that ``conda-build`` is up to date (``conda update conda-build``). This can be a source of difference between what is observed on the CI (install fresh into clean conda env) and a potentially stale local environment. You should also ensure that the channel order specified is the same as is applied in the CI for your ``conda build`` command. Refer to order applied in ``conda build`` step in pipeline yaml file. Priority decreases from left to right in your command line argument order. You should also ensure that your local `~/.condarc` file does not prioritise any unexpected/conflicting channels and that flag settings such as ``channel_priority: false`` are not utilised. Note that you can set ``--override-channels`` to your ``conda build`` command to prevent local `.condarc` files getting getting in the way.


Documentation on Github Pages
-----------------------------

The procedure below describes the steps to follow in order to publish documentation pages from a new repository in the Scipp organisation to the scipp.github.io website.

In this example, we assume that the new repository is named ``ess`` (the full path including organisation is hence ``scipp/ess``).

All documentation must go in a ``docs`` folder located at the root of the repository.
We use ``sphinx`` (and ``nbsphinx``) to transform ``.rst`` files and Jupyter notebooks into html pages.


#. Clone the ``ess`` repository: ``git clone git@github.com:scipp/ess.git``

#. Create a `gh-pages` branch on the repository with ``git checkout -b gh-pages``

#. On Github, go to the repository ``Settings > Pages`` (you will need admin rights to see the ``Settings`` tab). You should see that it is now saying "Your site is published at ``https://scipp.github.io/ess/``". It should also be saying it is using the ``gh-pages`` branch and the ``/root`` folder.

#. On your local machine, generate a ssh key, without an email or passphrase: ``ssh-keygen -t rsa -b 4096``. When prompted for a file name, use ``azure-ess-key`` (where ``ess`` is the name of the github repository).

#. Go to the repository's ``Settings > Deploy keys``. Add a new key, and copy the contents of the ``azure-ess-key.pub`` file. Remember to also give the key ``write`` access to the repository.

#. Go to the project page on Azure. Go to ``Pipelines > Library > Secure Files``. Upload the ``azure-ess-key`` (the private key of the pair without the ``.pub``) file as a secure file.

#. On your local machine, to back to where you cloned the ``ess`` git repository. Create a new branch starting from the ``main`` branch: ``git checkout main`` then ``git checkout -b deploy_docs``.

#. In that new branch, create a ``.azure-pipelines/documentation_deploy.yml`` script. The easiest way is to copy one from another repository, e.g. `ess-notebooks <https://github.com/scipp/ess-notebooks/blob/master/.azure-pipelines/documentation_deploy.yml>`_.

#. In that ``.azure-pipelines/documentation_deploy.yml`` file, replace the ``sshPublicKey`` with the contents of azure-ess-key.pub. Change the ``sshKeySecureFile`` to ``azure-ess-key``. Change ``git clone git@github.com:scipp/ess-notebooks`` to ``git clone git@github.com:scipp/ess``.

#. Push the changes to github (``git push origin deploy_docs``) and create a pull request. Hopefully, once the pull request is merged into ``main`` the pages will show up on `https://scipp.github.io/ess <https://scipp.github.io/ess>`_.

.. note::
  An alternative to this last step is, instead of waiting for the PR to be merged, one can just change the "branches trigger" in the ``main.yml`` and at the same time disable publishing of new packages as a temporary change.

.. note::
  Some useful links:

  * https://docs.github.com/en/developers/overview/managing-deploy-keys
  * https://docs.microsoft.com/en-us/azure/devops/pipelines/tasks/utility/install-ssh-key?view=azure-devops
