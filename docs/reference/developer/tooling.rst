Tooling
=======

This is a list of the tooling (i.e. compilers, static analysis, etc.) that are used in scipp and their corresponding versions.

Misc
~~~~

- CMake >= 3.13
- Conda

Note: Ubuntu users can use the `Kitware Repo <https://apt.kitware.com/>`_ to obtain the latest version.

Compilers
~~~~~~~~~

- GCC >= 7
- LLVM Clang >= 7
- XCode >= 10.2

Static Analysis and Formatters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``clang-format`` 10.0
- ``cmake-format`` 0.6.9
- ``flake8``
- ``yapf`` 0.29.0

``clang-format`` may be installed via the LLVM repositories.

``cmake-format``, ``flake8`` and ``yapf`` may be installed via ``pip``:

.. code-block::

  pip install \
    cmake-format==0.6.9 \
    flake8 \
    yapf==0.29.0
