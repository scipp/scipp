Coding conventions
==================

Formatting
----------

There are no explicit formatting conventions since we use ``clang-format`` (C++) and ``yapf`` (Python).

Integer types
-------------

* Do not use unsigned integers, including ``size_t``.
  Instead, use ``scipp::index`` which is a typedef for ``int64_t``.
  See `ES.107 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-subscripts>`_ for details.
  The free function ``scipp::size`` is provided as a free helper function to obtain a signed index for standard containers such as ``std::vector`` where the ``size`` method returns a forbidden ``size_t``.
* Do not use variable-width integers such as ``int`` or ``long``.
  These may be 32 bit or 64 bit depending on platform and compiler.
  Instead, use ``int32_t`` and ``int64_t``.
  Variable width ``int`` is acceptable in loops with known small extent, e.g., in ``for (int i=0; i<4; ++i)``.

Testing
-------

As a general rule all new functionality and bug fixes should be accompanied with sufficient unit tests to demonstrate that the feature or fix works as intended.
