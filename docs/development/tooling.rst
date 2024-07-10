Tooling
=======

This is a list of first party and third party tools used for developing Scipp.

Third Party
^^^^^^^^^^^

Compilers
~~~~~~~~~

All compilers can be installed through conda.
See ``conda/meta.yaml``.

- GCC [Linux]
- XCode [macOS]
- MSVC [Windows]

Static Analysis and Formatters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We use ``tox`` and ``pre-commit`` to do auto formatting and static analysis.
See in particular ``pre-commit-config.yaml`` for the list of used tools and versions.

Misc
~~~~

- CMake >= 3.21
- Conda

Note: Ubuntu users can use the `Kitware Repo <https://apt.kitware.com/>`_ to obtain the latest version.

First Party
^^^^^^^^^^^

There are several development tools in the ``tools`` directory at the top level of the Scipp repository.
See the readmes in the folder.
