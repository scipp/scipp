# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
# flake8: noqa
from skbuild import setup
from setuptools import find_packages


def get_version():
    import subprocess
    return subprocess.run(['git', 'describe', '--tags', '--abbrev=0'],
                          stdout=subprocess.PIPE).stdout.decode('utf8').strip()


setup(
    name='scipp',
    version=get_version(),
    description='Multi-dimensional data arrays with labeled dimensions',
    author='Scipp contributors (https://github.com/scipp)',
    url='https://scipp.github.io',
    license='BSD-3-Clause',
    packages=find_packages(where="src"),
    package_dir={'': 'src'},
    cmake_args=[
        '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF',
        '-DPYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())")',
        '-DPYTHON_LIBRARY=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var(\'LIBDIR\'))")'
    ],
    cmake_install_dir='src/scipp',
    include_package_data=True,
    python_requires='>=3.7',
    install_requires=[
        'numpy',
        'python-configuration',
    ])
