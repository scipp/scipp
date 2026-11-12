# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Scipp contributors (https://github.com/scipp)

from pathlib import Path

import scipp as sc


def test_installed_package_matches_checkout():
    """Catch stale cached packages even when their old tests still pass."""
    source = Path(__file__).resolve().parents[1] / 'src' / 'scipp'
    installed = Path(sc.__file__).resolve().parent
    patterns = ('**/*.py', '**/*.pyi', 'py.typed', 'visualization/templates/**/*')
    mismatches = []
    for pattern in patterns:
        for path in source.glob(pattern):
            if not path.is_file():
                continue
            relative = path.relative_to(source)
            # setuptools-scm generates this build artifact for wheels; it is
            # not source code and need not match the installed package.
            if relative == Path('_version.py'):
                continue
            target = installed / relative
            if not target.is_file() or target.read_bytes() != path.read_bytes():
                mismatches.append(str(relative))
    assert not mismatches, (
        f'Installed scipp at {installed} does not match {source}. '
        f'Rebuild the package. Stale or missing files: {sorted(mismatches)}'
    )
