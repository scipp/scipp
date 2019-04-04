Welcome to Scipp!
=================

**scipp** is heavily inspired by `xarray <xarray.pydata.org>`_.
While for many applications xarray is certainly more suitable than scipp, there is a number of features missing in other situations.
If you are missing one or several of the items on the following list in xarray, using scipp may be worth considering:

- Fully written in C++ better performance (for certain applications), in combination with a Python interface (**scippy**).
- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., scipp supports bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data.

Currently scipp is moving from its prototype phase into a more consolidated set of libraries.
The intention is to provide a generic core library (**scipp-core**), and more specific libraries (such as **scipp-neutron**).


Documentation
-------------

The documentation of **scipp** is under construction.

For the time being, please refer to the (slightly outdated) `design document <https://github.com/scipp/scipp/blob/master/doc/design.md>`_, or to the demo Jupyter notebooks in the Docker image:

.. code-block:: sh

   docker pull dmscid/dataset
   docker run -p 8888:8888 dmscid/dataset

Navigate to ``localhost:8888`` in your browser.
The relevant demo notebooks can be found in the ``demo/`` folder.
