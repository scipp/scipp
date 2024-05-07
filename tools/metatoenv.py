# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

##################################################################################
# WARNING! This file lives in the https://github.com/scipp/templates repository.
# It should only be modified there, and changes should then be propagated to other
# repositories in the Scipp organisation.
##################################################################################

import argparse
import os
import platform as _platform
from functools import reduce

parser = argparse.ArgumentParser(
    description='Generate a conda environment file from a conda recipe meta.yaml file'
)
parser.add_argument(
    '--dir', default='.', help='the directory where the conda meta.yaml file is located'
)
parser.add_argument(
    '--meta-file', default='meta.yaml', help='the name of the conda meta yaml file'
)
parser.add_argument(
    '--env-file', default='environment.yml', help='name of the output environment file'
)
parser.add_argument(
    '--env-name',
    '--name',
    default='',
    help='name of the environment that is stored inside the '
    'environment file (if no name is supplied, the name will be '
    'the same as the file name, without the .yml extension)',
)
parser.add_argument(
    '--channels',
    default='conda-forge',
    help='the conda channels to be added at the top of the '
    'environment file (to specify multiple channels, separate them '
    'with commas: --channels=conda-forge,scipp)',
)
parser.add_argument(
    '--platform',
    default=None,
    help='the platform (linux, osx, or win) on which the environment '
    'is destined to be created (if no platform is specified, the '
    'platform on which this script is running will be used)',
)
parser.add_argument(
    '--extra',
    default='',
    help='additional packages to be included in the environment '
    '(use commas to separate multiple entries, e.g. '
    '--extra=numpy,matplotlib,scipy)',
)
parser.add_argument(
    '--merge-with',
    default='',
    help='a second environment file that is to be merged with the '
    'one that would be created by the conda meta.yaml file alone',
)


def _indentation_level(string):
    """
    Count the number of leading spaces in a string.
    """
    return len(string) - len(string.lstrip(' '))


def _parse_yaml(text):
    """
    Parse yaml text and store into a dict.
    Description of algorithm:
     - if a line ends with `:`, we make a new nested dict. Current handle is then this
       new nested dict.
     - if a line does not end with `:`, but contains `:`, we set the value at the
       current handle in the dict
     - if a line does not end with `:`, and starts with `-`, we add the entry as a key
       in the dict and set the value to `None`
     - if the indentation level of the current line is less than the previous line, it
       implies we a closing a block. We iterate backwards in the file until we find a
       line with the same indent level and use the parent block of that line as our new
       handle
    """
    out = {}
    nspaces = 0
    handle = out
    path = []

    # Remove all comments and empty lines in text
    clean_text = []
    for line in text:
        line = line.rstrip(' \n')
        if len(line) > 0:
            line = line.split("#")[0]
            if len(line.strip()) > 0:
                clean_text.append(line.rstrip(' \n'))

    for i, line in enumerate(clean_text):
        if i > 0:
            line_indent = _indentation_level(line)
            if line_indent < _indentation_level(clean_text[i - 1]):
                current_indent = line_indent
                for p in path[::-1]:
                    if current_indent == p[1]:
                        ind = path.index(p)
                        handle = out
                        for j in range(ind):
                            handle = handle[path[j][0]]
                        path = path[:ind]
                        break

        if line.endswith(':'):
            key = line.strip(' -\n')
            handle[key] = {}
            handle = handle[key]
            nspaces = _indentation_level(line)
            path.append((key, nspaces))
        else:
            stripped = line.lstrip(' ')
            if stripped.startswith('-'):
                handle[stripped.strip(' -')] = None
            elif ':' in stripped:
                ind = stripped.find(':')
                handle[stripped[:ind]] = stripped[ind + 1 :]

    return out


def _jinja_filter(dependencies, platform):
    """
    Filter out deps for requested platform via jinja syntax.
    """
    out = {}
    for key, value in dependencies.items():
        if isinstance(value, dict):
            out[key] = _jinja_filter(value, platform)
        else:
            ok = True
            if key.count('[') > 0:
                left = key.find('[')
                right = key.find(']')
                if key.count(']') != key.count('[') or right < left:
                    raise RuntimeError(
                        "Bad preprocessing selector: "
                        "unmatched square brackets or closing bracket "
                        f"found before opening bracket: {key}"
                    )
                selector = key[left : right + 1]
                if selector.startswith('[not'):
                    if (platform in selector) or (
                        selector.replace('[not', '')[:-1].strip() in platform
                    ):
                        ok = False
                else:
                    if (platform not in selector) and (selector[1:-1] not in platform):
                        ok = False
                if ok:
                    key = key.replace(selector, '').strip(' \n')
            if ok:
                out[key] = value
    return out


def _merge_dicts(a, b, path=None):
    """
    Merges b into a.
    From: https://stackoverflow.com/a/7205107/13086629
    """
    if path is None:
        path = []
    for key in b:
        if key in a:
            if isinstance(a[key], dict) and isinstance(b[key], dict):
                _merge_dicts(a[key], b[key], [*path, str(key)])
            elif a[key] == b[key]:
                pass  # same leaf value
            else:
                raise Exception('Conflict at {}'.format('.'.join((*path, str(key)))))
        else:
            a[key] = b[key]
    return a


def _write_dict(d, file_handle, indent):
    for key, value in d.items():
        file_handle.write((" " * indent) + f"- {key}\n")
        if isinstance(value, dict):
            _write_dict(value, file_handle=file_handle, indent=indent + 2)


def main(metafile, envfile, envname, channels, platform, extra, mergewith):
    # Find current platform
    if platform is None:
        platform_mapping = {"Linux": "linux", "Darwin": "osx", "Windows": "win"}
        platform = platform_mapping[_platform.system()]

    # Read and parse metafile
    with open(metafile) as f:
        metacontent = f.readlines()
    meta = _parse_yaml(metacontent)

    # Merge three dicts into one
    meta_dependencies = meta["requirements:"]["build:"].copy()
    reduce(
        _merge_dicts,
        [meta_dependencies, meta["requirements:"]["run:"], meta["test:"]["requires:"]],
    )

    # Read file with additional dependencies
    if len(mergewith) > 0:
        with open(mergewith) as f:
            mergecontent = f.readlines()
        additional = _parse_yaml(mergecontent)
        additional_dependencies = additional["dependencies:"]
        _merge_dicts(meta_dependencies, additional_dependencies)
        channels = set(channels + list(additional["channels:"].keys()))

    # Add dependencies added via the command line
    for e in extra:
        if len(e) > 0:
            meta_dependencies[e] = None

    # Generate envname from output file name if name is not defined
    if len(envname) == 0:
        envname = os.path.splitext(envfile)[0]

    # Apply Jinja syntax filtering depending on platform
    meta_dependencies = _jinja_filter(meta_dependencies, platform=platform)

    # Write to output env file
    with open(envfile, "w") as out:
        out.write("##############################################################\n")
        out.write("# WARNING! This environment file was generated from the\n")
        out.write("# conda/meta.yaml file using the metatoenv.py tool.\n")
        out.write("# Do not update this file in-place.\n")
        out.write("# Use metatoenv.py to create a new file.\n")
        out.write("##############################################################\n\n")
        out.write(f"name: {envname}\n")
        out.write("channels:\n")
        for channel in channels:
            out.write(f"  - {channel}\n")
        out.write("dependencies:\n")
        _write_dict(meta_dependencies, file_handle=out, indent=2)


if __name__ == '__main__':
    args = parser.parse_args()

    channels = [args.channels]
    if "," in args.channels:
        channels = args.channels.split(",")
    extra = [args.extra]
    if "," in args.extra:
        extra = args.extra.split(",")

    main(
        metafile=os.path.join(args.dir, args.meta_file),
        envfile=args.env_file,
        envname=args.env_name,
        channels=channels,
        platform=args.platform,
        extra=extra,
        mergewith=args.merge_with,
    )
