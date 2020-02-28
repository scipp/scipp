How To...
=========

Test a trivial Python export
----------------------------

Python exports that do not add functionality above what the exported C++ function provides are classes as trivial.
Typically they can be identified by the pybind11 export `referencing the function itself <https://github.com/scipp/scipp/blob/2aca5d38a189beb233f4b3d730996dac45c3db78/python/variable.cpp#L399-L406>`_ or `via a trivial lambda <https://github.com/scipp/scipp/blob/2aca5d38a189beb233f4b3d730996dac45c3db78/python/variable.cpp#L377-L384>`_.

Given that the functionality is already tested by the C++ unit tests, all that is required is that the Python export exists and is callable with the expected arguments.
To do this a helper assertion ``assert_export`` is made available.
This assertion attempts to call the function with the arguments passed to it and checks if the specific exception raised when no valid overload is found is raised, if it is not then the export is deemed to be working.
Other exceptions are ignored as it is expected that some functions will throw when given empty input data.

An example of a test written for a trivial export is given below:

.. code-block:: python

    from .common import assert_export

    def test_abs_out():
      var = sc.Variable()
      assert_export(
        # Callable being tested
        sc.abs,
        # Positional argument
        var,
        # Keyword argument
        out=var)
