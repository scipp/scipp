# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import os
import argparse
import shutil
import subprocess
import multiprocessing
import sys
import time

parser = argparse.ArgumentParser(description='Build C++ library and run tests')
parser.add_argument('--prefix', default=os.environ.get('PREFIX', 'install'))
parser.add_argument('--site-packages-dir', default=os.environ.get('SP_DIR', ''))
parser.add_argument('--source_dir', default='.')
parser.add_argument('--build_dir', default='build')
parser.add_argument('--caching', action='store_true', default=False)


def run_command(cmd, shell):
    """
    Run a command (supplied as a list) using subprocess.check_call
    """
    os.write(1, "{}\n".format(' '.join(cmd)).encode())
    return subprocess.check_call(cmd, stderr=subprocess.STDOUT, shell=shell)


def main(*,
         prefix='install',
         site_packages_dir,
         build_dir='build',
         source_dir='.',
         caching=False):
    """
    Platform-independent function to run cmake, build, install and C++ tests.
    """

    # Get the platform name: 'linux', 'darwin' (osx), or 'win32'.
    platform = sys.platform

    # Set up absolute directory paths
    source_dir = os.path.abspath(source_dir)
    prefix = os.path.abspath(prefix)
    build_dir = os.path.abspath(build_dir)

    # Default options
    shell = False
    parallel_flag = '-j{}'.format(multiprocessing.cpu_count())
    build_config = ''

    # Some flags use a syntax with a space separator instead of '='
    use_space = ['-G', '-A']

    # Default cmake flags
    ipo = os.getenv('CMAKE_INTERPROCEDURAL_OPTIMIZATION', 'OFF')
    cmake_flags = {
        '-G': 'Ninja',
        '-DPython_EXECUTABLE': shutil.which("python"),
        '-DCMAKE_INSTALL_PREFIX': prefix,
        '-DSITE_PACKAGES_DIR': site_packages_dir,
        '-DWITH_CTEST': 'OFF',
        '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION': ipo,
        '-DFULL_BUILD': 1,
    }

    if platform == 'darwin':
        # Note 10.14 is the minimum supported osx version
        # conda-build otherwise defaults to 10.9
        osxversion = os.getenv('OSX_VERSION', '10.14')
        cmake_flags.update({
            '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION': 'OFF',
            '-DCMAKE_OSX_DEPLOYMENT_TARGET': osxversion
        })
    if platform == 'win32':
        cmake_flags.update({'-G': 'Visual Studio 16 2019', '-A': 'x64'})
        # clcache conda installed to env Scripts dir in env if present
        scripts = os.path.join(os.environ.get('CONDA_PREFIX'), 'Scripts')
        if caching and os.path.exists(os.path.join(scripts, 'clcache.exe')):
            cmake_flags.update({'-DCLCACHE_PATH': scripts})
        shell = True
        build_config = 'Release'
        # cmake --build --parallel is detrimental to build performance on
        # windows, see https://github.com/scipp/scipp/issues/2078 for
        # details
        build_flags = []
    else:
        # For other platforms we do want to add the parallel build flag.
        build_flags = [parallel_flag]

    if len(build_config) > 0:
        build_flags += ['--config', build_config]

    # Parse cmake flags
    flags_list = []
    for key, value in cmake_flags.items():
        if key in use_space:
            flags_list += [key, value]
        else:
            flags_list.append('{}={}'.format(key, value))

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)

    # Run cmake
    run_command(['cmake'] + flags_list + [source_dir], shell=shell)

    # Show cmake settings
    run_command(['cmake', '-B', '.', '-S', source_dir, '-LA'], shell=shell)

    # Compile benchmarks, C++ tests, and python library
    start = time.time()
    run_command(['cmake', '--build', '.', '--target', 'install'] + build_flags,
                shell=shell)
    end = time.time()
    print('Compilation took ', end - start, ' seconds')

    # Run C++ tests
    for test in [
            'scipp-common-test', 'scipp-units-test', 'scipp-core-test',
            'scipp-variable-test', 'scipp-dataset-test'
    ]:
        run_command([os.path.join('bin', build_config, test)], shell=shell)


if __name__ == '__main__':
    args = parser.parse_args()
    main(prefix=args.prefix,
         site_packages_dir=args.site_packages_dir,
         build_dir=args.build_dir,
         source_dir=args.source_dir,
         caching=args.caching)
