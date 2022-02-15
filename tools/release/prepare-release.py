# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
"""
Use this script right before a release to:
- Add a new entry to the version-selection menu in the documentation.
- Update headings in release notes and add new empty sections for next release.

BEFORE running this script update the list of versions and targets to include the
new desired version (or remove old unused versions).
"""

import pathlib
from datetime import datetime
from string import Template


def make_version_select(root):
    version_select = f'{root}/docs/_templates/topbar/launchbuttons.html'

    with open(f'{root}/tools/release/templates/launchbuttons.html', 'r') as f:
        contents = f.read()

    versions = ['0.12', '0.11', '0.10', '0.9', '0.8', '0.7', '0.6', '0.5', '0.4', '0.3']
    targets = [
        '0.12.1', '0.11.0', '0.10.1', '0.9.0', '0.8.4', '0.7.1', '0.6.1', '0.5.0',
        '0.4.0', '0.3.0'
    ]

    entries = []
    for version, target in zip(versions, targets):
        entries.append(f"""        <a class="dropdown-buttons"
            href="https://scipp.github.io/release/{target}"><button type="button"
                class="btn btn-secondary topbarbtn">v{version}</button></a>""")

    contents = Template(contents).substitute(releases='\n'.join(entries))
    contents = '<!-- DO NOT EDIT! This file is created using tools/release/prepare-release.html. -->\n' + contents  # noqa: E501

    with open(version_select, "w") as f:
        f.write(contents)


def update_release_notes(root):
    release_notes = f'{root}/docs/about/release-notes.rst'
    with open(release_notes, 'r') as f:
        contents = f.readlines()
    current = contents[5]
    version = current.split()[0][1:].split('.')
    major = int(version[0])
    minor = int(version[1])
    if 'unreleased' not in current:
        raise RuntimeError("Expected current unreleased heading in line 5")
    now = datetime.now()
    contents[5] = current.replace('unreleased', f'{now.strftime("%B")} {now.year}')
    contents[6] = '-' * (len(contents[5]) - 1) + '\n'
    with open(f'{root}/tools/release/templates/release-notes.rst', 'r') as f:
        lines = f.readlines()
    lines.insert(0, f'v{major}.{minor+1}.0 (unreleased)\n')
    lines.insert(1, '-' * (len(lines[0]) - 1) + '\n')
    contents.insert(5, ''.join(lines))

    with open(release_notes, "w") as f:
        contents = "".join(contents)
        f.write(contents)


if __name__ == '__main__':
    filedir = pathlib.Path(__file__).parent.resolve()
    tools = filedir.parent.resolve()
    root = tools.parent.resolve()
    make_version_select(root)
    update_release_notes(root)
