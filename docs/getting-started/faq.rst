.. _faq:

Frequently Asked Questions
==========================

Specific help with using Scipp
------------------------------

For help on specific issues with using Scipp, you should first visit the
`discussions <https://github.com/scipp/scipp/discussions>`_
page to see if the problem you are facing has already been met/solved in the community.

If you cannot find an answer, simply start a new discussion there.

General
-------

Scipp is using more and more memory / the Jupyter kernel crashes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may be a victim of IPython's (and thus Jupyter's) `Output caching system <https://ipython.readthedocs.io/en/stable/interactive/reference.html?highlight=previous#output-caching-system>`_.
For example, IPython will hold on to a reference of any variable (Scipp or other) written at the end of a cell (named or unnamed).
``print(Out)`` can be used to show the cached outputs.
Since we frequently use and recommend to use Jupyter's feature to display rich HTML output (provided by an object's ``_repr_html_`` method), many large objects may end up in this cache.
Note that deleting your own reference (such as using ``del my_array``) will *not* free the memory, since the IPython output cache holds another reference.

There are two solutions (as described in the IPython `Output caching system <https://ipython.readthedocs.io/en/stable/interactive/reference.html?highlight=previous#output-caching-system>`_ documentation):

1. Free the cache manually, on demand, for example:

   .. code::

      %reset out

2. Decrease the cache size or disable it completely by editing your ``ipython_kernel_config.py`` and setting:

   .. code::

      c.InteractiveShell.cache_size = 0

   See `Python configuration files <https://ipython.readthedocs.io/en/stable/config/intro.html#python-configuration-files>`_ and `InteractiveShell.cache_size <https://ipython.readthedocs.io/en/stable/config/options/index.html#configtrait-InteractiveShell.cache_size>`_ in the IPython documentation for details.

Where does Scipp's behavior differ from Xarray's behavior?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Similar to Pandas, Xarray uses *indexes* for automatically aligning data in operations based on their coordinates.
  Binary operations in Xarray may thus return the result for the intersection of the input coordinates, i.e., those with matching coordinate values.
  This leads to a number of cases with surprising behavior and can also be expensive when unintentional reordering of large data occurs.
  Scipp takes a much simpler approach by simply comparing the arrays of coordinate values, raising an error if there is a mismatch.
  Reordering is not supported.
- Scipp's attributes behave similar to Xarray's non-index coordinates.
  A minor difference is that Scipp handles a missing attribute as a mismatch and will thus drop the attribute.
  Scipp thus ensures ``(a + b) + c == a + (b + c)``, which is not the case in Xarray (when considering non-index coordinates).

Why is Xarray not enough?
~~~~~~~~~~~~~~~~~~~~~~~~~

For our application (handling of neutron-scattering data, which is so far mostly done using `Mantid <https://mantidproject.org>`_), Xarray is currently missing a number of features:

- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data.
  More concretely, this is essentially a 1-D (or N-D) array of random-length lists, with very small list entries.
  This type of data arises in time-resolved detection of neutrons in pixelated detectors.
- Written in C++ for performance opportunities, in particular also when interfacing with our extensive existing C++ codebase.

Why are you not just contributing to Xarray?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a valid criticism and at times we still feel uncertain about this choice.
The decision was made in the context of a larger project which needed to come to a conclusion within a couple of years, including all the items listed in the previous FAQ entry.
Essentially we felt that the list of additional requirements on top of what Xarray provides was too long.
Effecting/contributing such fundamental changes to an existing framework is a long process and likely not obtained within the given time frame.
Furthermore, some of the requirements are unlikely to be obtainable within Xarray.

We should note that at least some of our additional requirements, in particular physical units, are being pursued also by the Xarray developers.

Plotting
--------

How can I set axis limits when creating a plot?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This can be achieved indirectly with Scipp's `generic slicing functionality <../user-guide/slicing.rst>`_ and `label-based indexing <../user-guide/slicing.ipynb#Label-based-indexing>`_ in particular.
Example:

.. code-block:: python

  array.plot()  # plot with full x range
  array['x', 100:200].plot()  # plot 100 points starting at offset 100
  start = 1.2 * sc.Unit('m')
  stop = 1.3 * sc.Unit('m')
  array['x', start:stop].plot()  # plot everything between 1.2 and 1.3 meters

Installation
------------

I can't import Scipp!
~~~~~~~~~~~~~~~~~~~~~

On Windows, after installing Scipp using ``conda``, attempting to ``import scipp`` may sometimes fail with

.. code-block:: python

  > import scipp as sc
  > ImportError: DLL load failed: The specified module could not be found.

This issue is Windows specific and fixing it requires downloading and installing a recent version of the Microsoft Visual C++ Redistributable for
Visual Studio 2019.
It can be downloaded from `Microsoft's official site <https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0>`_.
