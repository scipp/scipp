# ruff: noqa: S603, S607

import os
import subprocess
import sys

shell = sys.platform == 'win32'

os.chdir(os.path.dirname(os.path.realpath(__file__)))
build_dir = os.path.relpath('build')
if not os.path.exists(build_dir):
    os.makedirs(build_dir)
os.chdir(build_dir)

cmake_flags = [
    f'-DPKG_VERSION={os.environ["PKG_VERSION"]}',
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_VERBOSE_MAKEFILE=ON',
]

if sys.platform == 'darwin':
    cmake_flags.append(
        f'-DCMAKE_OSX_DEPLOYMENT_TARGET={os.getenv("OSX_VERSION", "10.14")}'
    )
elif sys.platform == 'win32':
    cmake_flags.extend(['-G', 'Visual Studio 16 2019'])

subprocess.check_call(
    ['cmake', *cmake_flags, '..'], stderr=subprocess.STDOUT, shell=shell
)
# Show cmake settings
subprocess.check_call(
    ['cmake', '-B', '.', '-S', '..', '-LA'], stderr=subprocess.STDOUT, shell=shell
)
# TODO For some reason transitive dependencies such as boost include dirs to not work
# on Windows
if sys.platform != 'win32':
    subprocess.check_call(
        ['cmake', '--build', '.'], stderr=subprocess.STDOUT, shell=shell
    )
