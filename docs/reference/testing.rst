Testing code with Scipp
=======================

Assertions
----------

It is possible to write tests in terms of regular functions provided by Scipp such as :func:`scipp.identical` or :func:`scipp.allclose`.
While this works, error messages produced with those functions contain little information and it is often necessary to inspect the compared objects manually in case of a test failure.

For this reason, the :mod:`scipp.testing.assertions` (also available in :mod:`scipp.testing`) module provides a function to compare objects:

.. code-block:: python

    scipp.testing.assert_identical(a, b)

It is intended for use with pytest and is equivalent to

.. code-block:: python

    assert scipp.identical(a, b, equal_nan=True)

but produces errors that precisely indicate why the inputs are not equal.

There currently is no equivalent for :func:`scipp.allclose`.

Configuration
~~~~~~~~~~~~~

To get the most out of it ``assert_identical``, you need to configure pytest with the following.
For example, by placing it in your ``conftest.py``:

.. code-block:: python

    pytest.register_assert_rewrite('scipp.testing.assertions')

This tells pytest that ``scipp.testing.assertions`` contains test assertions.
pytest will then change those assertions the same way as assertions in test files to produce more detailed error messages.

On top, we recommend the following (also in ``conftest.py``) to improve errors messages further:

.. code-block:: python

    def pytest_assertrepr_compare(op: str, left: Any, right: Any) -> List[str]:
        if isinstance(left, sc.Unit) and isinstance(right, sc.Unit):
            return [f'Unit({left}) {op} Unit({right})']
        if isinstance(left, sc.DType) or isinstance(right, sc.DType):
            return [f'{left!r} {op} {right!r}']

.. seealso::

    Scipp's own `conftest.py <https://github.com/scipp/scipp/blob/main/tests/conftest.py>`_.

Input generation
----------------

Scipp's containers have many flexible components that can produce many different and sometimes unexpected combinations.
It is difficult to write tests that cover all cases by hand.
So Scipp provides tools to generate data structures in the form of `Hypothesis <https://hypothesis.readthedocs.io/en/latest/>`_ strategies in :mod:`scipp.testing.strategies`.
See in particular :func:`scipp.testing.strategies.variables`.
Support for anything beyond variables is currently limited and binned data is not supported.

To use the strategies, install hypothesis and, in pytest, write, e.g.,

.. code-block:: python

    from hypothesis import given
    import scipp.testing.strategies as scst

    @given(scst.variables())
    def test_abs_preserves_shape(var):
        assert abs(var).shape == var.shape

The ``variables`` strategy generates arbitrary non-binned variables with different units, dims, shapes, dtypes, values, and variances.
It has several arguments that can be used to steer generation.
For example, to generate only two-dimensional variables with floating point dtypes, use

.. code-block:: python


    from hypothesis import given, strategies as st
    import scipp.testing.strategies as scst

    @given(scst.variables(ndim=2,
                          dtype=st.sampled_from(('float64', 'float32'))))
    def test_mean_reduces_ndim(var):
        assert var.mean(dim=var.dims[0]).ndim == 1
