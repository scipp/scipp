.. _runtime_configuration:

Runtime Configuration
=====================

The ``scipp`` Python module supports several configuration options which may be set based on preference.
Most of these are related to layout of items in Jupyter Notebooks and plotting.

The location of the configuration file varies between operating systems.
To determine where it is located on a particular installation the following Python may be used:

.. code-block:: python

  import scipp as sc
  print(sc.user_configuration_filename)

When the ``scipp`` module is imported and no configuiration file exists a default one is created.
If a configuration file already exists it will not be modified, i.e. new options will not automatically be added to it when Scipp is updated.

The fields in the configuration file are not strictly defined and may vary between Scipp versions.
For this reason it is advised to browse this file to determine what options are available.
