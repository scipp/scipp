# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from pathlib import Path
import os
import sys
import subprocess
import tempfile

shell = sys.platform == 'win32'

os.chdir(os.path.dirname(os.path.realpath(__file__)))

with tempfile.TemporaryDirectory() as build_dir:
    build_dir = os.environ.get('DOCS_BUILD_DIR', build_dir)
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    with tempfile.TemporaryDirectory() as work_dir:
        subprocess.check_call([
            'python', 'build_docs.py', '--builder=html', f'--prefix={build_dir}',
            f'--work_dir={work_dir}'
        ],
                              stderr=subprocess.STDOUT,
                              shell=shell)

    with tempfile.TemporaryDirectory() as work_dir:
        subprocess.check_call([
            'python', 'build_docs.py', '--builder=doctest', f'--prefix={build_dir}',
            f'--work_dir={work_dir}', '--no-setup'
        ],
                              stderr=subprocess.STDOUT,
                              shell=shell)

    # Remove Jupyter notebooks used for documentation build,
    # they are not accessible and create size bloat.
    # However, keep the ones in the `_sources` folder,
    # as the download buttons links to them.
    sources_dir = os.path.join(build_dir, '_sources')
    for path in Path(build_dir).rglob('*.ipynb'):
        if not str(path).startswith(sources_dir):
            os.remove(path)
