Tooling
=======

This is a list of first party and third party tools used for developing Scipp.

Third Party
^^^^^^^^^^^

Compilers
~~~~~~~~~

The ``dev`` pixi environment provides the C++ toolchain (conda ``cxx-compiler``) automatically on Linux and macOS.
On Windows, MSVC from Visual Studio is used (CI sets it up via ``vs-shell``).
Release conda packages use the compilers from ``conda/meta.yaml``.

- GCC (conda ``cxx-compiler``) [Linux]
- Clang (conda ``cxx-compiler``) [macOS]
- MSVC [Windows]

Static Analysis and Formatters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We use ``pixi`` and ``pre-commit`` to do auto formatting and static analysis.
See in particular ``pre-commit-config.yaml`` for the list of used tools and versions.

Misc
~~~~

- CMake >= 3.28 (provided by the ``dev`` pixi environment)
- `Pixi <https://pixi.sh>`_

First Party
^^^^^^^^^^^

There are several development tools in the ``tools`` directory at the top level of the Scipp repository.
See the readmes in the folder.
