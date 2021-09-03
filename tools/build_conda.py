# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import os
import shutil
import glob
import sys
import build_cpp


class FileMover():
    def __init__(self, source_root, destination_root):
        self.source_root = source_root
        self.destination_root = destination_root

    def move_file(self, src, dst):
        os.write(1, "move {} {}\n".format(src, dst).encode())
        shutil.move(src, dst)

    def move(self, src, dst):
        src = os.path.join(self.source_root, *src)
        dst = os.path.join(self.destination_root, *dst)
        if '*' in dst:
            dst = glob.glob(dst)[-1]
        if '*' in src:
            for f in glob.glob(src):
                self.move_file(f, dst)
        else:
            self.move_file(src, dst)


if __name__ == '__main__':

    # Search for a defined SCIPP_INSTALL_PREFIX env variable.
    # If it exists, it points to a previously built target and we simply move
    # the files from there into the conda build directory.
    # If it is undefined, we build the C++ library by calling main() from
    # build_cpp.
    source_root = os.environ.get('SCIPP_INSTALL_PREFIX')
    if source_root is None:
        source_root = os.path.abspath('scipp_install')
        build_cpp.main(prefix=source_root)
    destination_root = os.environ.get('CONDA_PREFIX')

    # Create a file mover to place the built files in the correct directories
    # for conda build.
    m = FileMover(source_root=source_root, destination_root=destination_root)

    # Depending on the platform, directories have different names.
    if sys.platform == "win32":
        lib_dest = 'lib'
        dll_src = os.path.join("lib", "scipp")
        dll_dest = os.path.join("lib", "scipp")
        lib_src = 'Lib'
        inc_src = 'include'
    else:
        lib_dest = os.path.join('lib', 'python*')
        dll_src = None
        dll_dest = None
        lib_src = 'lib'
        inc_src = 'include'

    m.move(['scipp'], [lib_dest])
    if dll_src is not None:
        m.move([dll_src, 'scipp-*'], [dll_dest])
    m.move([lib_src, '*scipp*'], [lib_src])
    m.move([lib_src, 'cmake', 'scipp'], [lib_src, 'cmake'])
    m.move([inc_src, 'scipp*'], [inc_src])
