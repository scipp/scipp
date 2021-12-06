# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
    args = ['-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF']
    return args


setup(name='scipp',
      version=get_version(),
      description='Multi-dimensional data arrays with labeled dimensions',
      author='Scipp contributors (https://github.com/scipp)',
      url='https://scipp.github.io',
      license='BSD-3-Clause',
      packages=find_packages(where="src"),
      package_dir={'': 'src'},
      cmake_args=get_cmake_args(),
      cmake_install_dir='src/scipp',
      include_package_data=True,
      python_requires='>=3.7',
      install_requires=[
          'appdirs',
          'graphlib-backport',
          'numpy>=1.20',
          'python-configuration',
          'pyyaml',
      ],
      extras_require={
          "test": ["pytest"],
          'all': ['h5py', 'scipy', 'python-graphviz'],
          'interactive': [
              'ipykernel==6.3.1', 'ipympl', 'ipython', 'ipywidgets', 'matplotlib-base',
              'jupyterlab', 'jupyterlab-widgets', 'jupyter_nbextensions_configurator',
              'nodejs', 'pythreejs'
          ],
      })
