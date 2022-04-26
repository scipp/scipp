# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from skbuild import setup
from setuptools import find_packages


def get_version():
    import subprocess
    return subprocess.run(['git', 'describe', '--tags', '--abbrev=0'],
                          stdout=subprocess.PIPE).stdout.decode('utf8').strip()


def get_cmake_args():
    # Note: We do not specify '-DCMAKE_OSX_DEPLOYMENT_TARGET' here. It is set using the
    # MACOSX_DEPLOYMENT_TARGET environment variable in the github workflow. The reason
    # is that I am not sure if cibuildwheel uses this for anything else apart from
    # configuring the actual build.
    return []


long_description = """# Multi-dimensional data arrays with labeled dimensions

*A Python library enabling a modern and intuitive way of working with scientific data in Jupyter notebooks*

**scipp** is heavily inspired by [xarray](https://xarray.pydata.org>).
It enriches raw NumPy-like multi-dimensional arrays of data by adding named dimensions and associated coordinates.
Multiple arrays can be combined into datasets.
While for many applications xarray is certainly more suitable (and definitely much more matured) than scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using scipp may be worth considering:

- **Physical units** are stored with each data or coord array and are handled in arithmetic operations.
- **Propagation of uncertainties**.
- Support for **histograms**, i.e., **bin-edge axes**, which are by 1 longer than the data extent.
- Support for scattered data and **non-destructive binning**.
  This includes first and foremost **event data**, a particular form of sparse data with arrays of random-length lists, with very small list entries.
- Support for **masks stored with data**.
- Internals written in C++ for better performance (for certain applications), in combination with Python bindings.
"""  # noqa #501

setup(name='scipp',
      version=get_version(),
      description='Multi-dimensional data arrays with labeled dimensions',
      long_description=long_description,
      long_description_content_type='text/markdown',
      author='Scipp contributors (https://github.com/scipp)',
      url='https://scipp.github.io',
      license='BSD-3-Clause',
      packages=find_packages(where="src"),
      package_dir={'': 'src'},
      cmake_args=get_cmake_args(),
      cmake_install_dir='src/scipp',
      include_package_data=True,
      python_requires='>=3.8',
      install_requires=['confuse', 'graphlib-backport', 'numpy>=1.20'],
      extras_require={
          "test": ["pytest", "matplotlib", "xarray", "pandas", "pythreejs"],
          'all': ['h5py', 'scipy>=1.7.0', 'graphviz', 'hypothesis', 'pooch'],
          'interactive': [
              'ipympl', 'ipython', 'ipywidgets', 'matplotlib', 'jupyterlab',
              'jupyterlab-widgets', 'jupyter_nbextensions_configurator', 'nodejs',
              'pythreejs'
          ],
      })
