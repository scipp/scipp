# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import argparse
import os
import pathlib
import subprocess
import sys

parser = argparse.ArgumentParser(description='Build doc pages with sphinx')
parser.add_argument('--prefix', default='build')
parser.add_argument('--work_dir', default='.doctrees')


def get_abs_path(path, root):
    if os.path.isabs(path):
        return path
    else:
        return os.path.join(root, path)


if __name__ == '__main__':

    args = parser.parse_args()

    docs_dir = pathlib.Path(__file__).parent.absolute()
    work_dir = get_abs_path(path=args.work_dir, root=docs_dir)
    prefix = get_abs_path(path=args.prefix, root=docs_dir)

    # Build the docs with sphinx-build
    status = subprocess.check_call(
        ['sphinx-build', '-d', work_dir, docs_dir, prefix],
        stderr=subprocess.STDOUT,
        shell=sys.platform == "win32")
