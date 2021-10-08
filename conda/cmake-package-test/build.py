import os
import sys
import subprocess

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
        f'-DCMAKE_OSX_DEPLOYMENT_TARGET={os.getenv("OSX_VERSION", "10.14")}')

subprocess.check_call(['cmake'] + cmake_flags + ['..'],
                      stderr=subprocess.STDOUT,
                      shell=shell)
# Show cmake settings
subprocess.check_call(['cmake', '-B', '.', '-S', '..', '-LA'],
                      stderr=subprocess.STDOUT,
                      shell=shell)
subprocess.check_call(['cmake', '--build', '.'], stderr=subprocess.STDOUT, shell=shell)
