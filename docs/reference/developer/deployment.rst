.. _deployment:

Deployment
==========

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

#. In that new branch, create a ``.azure-pipelines/documentation_deploy.yml`` script. The easiest way is to copy one from another repository, e.g. `ess-notebooks <https://github.com/scipp/ess-notebooks/blob/main/.azure-pipelines/documentation_deploy.yml>`_.

#. In that ``.azure-pipelines/documentation_deploy.yml`` file, replace the ``sshPublicKey`` with the contents of azure-ess-key.pub. Change the ``sshKeySecureFile`` to ``azure-ess-key``. Change ``git clone git@github.com:scipp/ess-notebooks`` to ``git clone git@github.com:scipp/ess``.

#. Push the changes to github (``git push origin deploy_docs``) and create a pull request. Hopefully, once the pull request is merged into ``main`` the pages will show up on `https://scipp.github.io/ess <https://scipp.github.io/ess>`_.

.. note::
  An alternative to this last step is, instead of waiting for the PR to be merged, one can just change the "branches trigger" in the ``main.yml`` and at the same time disable publishing of new packages as a temporary change.

.. note::
  Some useful links:

  * https://docs.github.com/en/developers/overview/managing-deploy-keys
  * https://docs.microsoft.com/en-us/azure/devops/pipelines/tasks/utility/install-ssh-key?view=azure-devops
