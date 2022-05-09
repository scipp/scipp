Tooling
=======

This is a list of the tooling (i.e. compilers, static analysis, etc.) that are used in scipp and their corresponding versions.

Misc
~~~~

- CMake >= 3.16
- Conda

Note: Ubuntu users can use the `Kitware Repo <https://apt.kitware.com/>`_ to obtain the latest version.

Compilers
~~~~~~~~~

- GCC >= 9
- XCode >= 12.4
- MSVC >= 16 (2019)

Static Analysis and Formatters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``clang-format`` 10.0
- ``cmake-format`` 0.6.9
- ``flake8``
- ``yapf`` 0.32.0

``clang-format`` may be installed via the LLVM repositories.

``cmake-format``, ``flake8`` and ``yapf`` may be installed via ``pip``:

.. code-block::

  pip install \
    cmake-format==0.6.9 \
    flake8 \
    yapf==0.32.0
