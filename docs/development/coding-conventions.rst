Coding conventions
==================

Integer types
-------------

* Do not use unsigned integers, including ``size_t``.
  Instead, use ``scipp::index`` which is a typedef for ``int64_t``.
  See `ES.107 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-subscripts>`_ for details.
  The free function ``scipp::size`` is provided as a free helper function to obtain a signed index for standard containers such as ``std::vector`` where the ``size`` method returns a forbidden ``size_t``.
* Do not use variable-width integers such as ``int`` or ``long``.
  These may be 32 bit or 64 bit depending on platform and compiler.
  Instead, use ``int32_t`` and ``int64_t``.
  Variable width ``int`` is acceptable in loops with known small extent, e.g., in ``for (int i=0; i<4; ++i)``.

Testing
-------

As a general rule all new functionality and bug fixes should be accompanied with sufficient unit tests to demonstrate that the feature or fix works as intended.

Formatting
----------

There are no explicit formatting conventions since we use ``clang-format`` (C++) and ``ruff`` (Python).

Docstrings
~~~~~~~~~~

The exception to this are Python docstrings, for which we use the
`NumPy docstring format <https://www.sphinx-doc.org/en/master/usage/extensions/example_numpy.html>`_.
We use a tool to automatically insert type hints into the docstrings.
Our format, therefore, deviates from the default NumPy example given by the link above.
The example below shows how docstrings should be laid out in Scipp including spacing and punctuation.

.. code-block:: python

    def foo(x: int, y: float) -> float:
        """Short description.

        Long description.

        With multiple paragraphs.

        Warning
        -------
        Be careful!

        Parameters
        ----------
        x:
            First input.
        y:
            Second input.

        Returns
        -------
        :
            The result.

        Raises
        ------
        ValueError
            If the input is bad.
        IndexError
            If some lookup failed.

        See Also
        --------
        scipp.fold:
            For reshaping into multiple dimensions.
        scipp.flatten:
            For removing dimensions.

        Examples
        --------
        This is how to use it:

          >>> sc.arange('x', 3)
          <scipp.Variable> (x: 3)      int64  [dimensionless]  [0, 1, 2]

        And also:

          >>> sc.linspace('x', 1, 4, 2)
          <scipp.Variable> (x: 2)    float64  [dimensionless]  [1, 4]
        """

The order of sections is fixed as shown in the example.

* **Short description** (*required*) A single sentence describing the purpose of the function / class.
* **Long description** (*optional*) One or more paragraphs of detailed explanations.
  Can include additional sections like ``Warning`` or ``Hint``.
* **Parameters** (*required for functions*) List of all function arguments including their name but not their type.
  Listing arguments like this can seem ridiculous if the explanation is as devoid of content as in the example.
  But it is still required in order for sphinx to show the types.
* **Returns** (*required for functions*) Description of the return value.
  Required for the same reason as the parameter list.

  For a single return value, neither a name nor type should be given.
  But a colon is required as in the example above in order to produce proper formatting.

  For multiple return values, to produce proper formatting,
  both name and type must by given even though the latter repeats the type annotation:

  .. code-block:: python

    """
    Returns
    -------
    n: int
        The first return value.
    z: float
        The second return value.
    """

* **Raises** (*optional*) We generally do not document what exceptions can be raised from a function.
  But if there are some important cases, this section can list those exceptions with an explanation
  of when the exception is raised.
  The exception type is required.
  Note that there are no colons here.
* **See Also** (*optional*) List of related functions and / or classes.
  The function / class name should include the module it is in but without reST markup.
  For simple cases, the explanation can be left out.
  In this case, the colon should be omitted as well and multiple entries must be separated by commas.
* **Examples** (*optional*) Example code given using ``>>>`` as the Python prompt.
  May include text before, after, and between code blocks.
  Note the spacing in the example.

Some functions can be sufficiently described by a single sentence.
In this case, the 'Parameters' and 'Returns' sections may be omitted and the docstring should be laid out on a single line.
If it does not fit on a single line, it is too complicated.
For example

.. code-block:: python

    def ndim(self) -> int:
        """Returns the number of dimensions."""

But note that the argument types are not shown in the rendered documentation!

Plots in docstrings
^^^^^^^^^^^^^^^^^^^

Code blocks in docstrings can produce plots.
This works either via placing
`matplotlib.sphinxext.plot_directive <https://matplotlib.org/stable/api/sphinxext_plot_directive_api.html>`_
explicitly or by automatically placing those directives via :mod:`scipp.sphinxext.autoplot`.
See the module documentation for more information.
