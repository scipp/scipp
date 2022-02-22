# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

##################################################################################
# WARNING! This file lives in the https://github.com/scipp/templates repository.
# It should only be modified there, and changes should then be propagated to other
# repositories in the Scipp organisation.
##################################################################################

import os
import argparse
import sys
import subprocess

parser = argparse.ArgumentParser(
    description='Generate a conda environment file from a conda recipe meta.yaml file')
parser.add_argument('--dir',
                    default='.',
                    help='the directory where the conda meta.yaml file is located')
parser.add_argument('--env-file',
                    default='environment.yml',
                    help='name of the output environment file')
parser.add_argument('--env-name',
                    '--name',
                    default='',
                    help='name of the environment that is stored inside the '
                    'environment file (if no name is supplied, the name will be '
                    'the same as the file name, without the .yml extension)')
parser.add_argument('--channels',
                    default='conda-forge',
                    help='the conda channels to be added at the top of the '
                    'environment file (to specify multiple channels, separate them '
                    'with commas: --channels=conda-forge,scipp)')

# def _run(command, shell, executable=None, return_output=False):
#     print(command)
#     if return_output:
#         return subprocess.check_output(command,
#                                        stderr=subprocess.STDOUT,
#                                        shell=shell,
#                                        executable=executable).decode()
#     else:
#         subprocess.check_call(command,
#                               stderr=subprocess.STDOUT,
#                               shell=shell,
#                               executable=executable)


def main(condadir, envfile, envname, channels):

    shell = sys.platform == 'win32'

    # Generate envname from output file name if name is not defined
    if len(envname) == 0:
        envname = os.path.splitext(envfile)[0]

    out = subprocess.check_output(
        ['conda', 'debug', '--channel', 'conda-forge', condadir],
        stderr=subprocess.STDOUT,
        shell=shell).decode()
    print(out)

    lines = out.split('\n')
    env_setup_file = None
    for line in reversed(lines):
        if all(x in line for x in ['cd', 'conda-bld', '&&']):
            print(line)
            env_setup_file = line.split()[-1]
            break

    if env_setup_file is None:
        raise RuntimeError("No setup file found in output from conda debug.")
    print(env_setup_file)

    with open(env_setup_file, 'r') as f:
        setup_file_contents = f.readlines()

    history_file = None
    for line in reversed(setup_file_contents):
        if line.startswith('conda activate'):
            print(line)
            history_file = os.path.join(line.split()[-1].replace('"', ''), 'conda-meta',
                                        'history')
            break

    if history_file is None:
        raise RuntimeError("No history file found in the conda debug folder.")

    with open(history_file, 'r') as f:
        history = f.readlines()

    with open(envfile, "w") as out:
        out.write("##############################################################\n")
        out.write("# WARNING! This environment file was generated from the\n")
        out.write("# conda/meta.yaml file using the metatoenv.py tool.\n")
        out.write("# Do not update this file in-place.\n")
        out.write("# Use metatoenv.py to create a new file.\n")
        out.write("##############################################################\n\n")
        out.write("name: {}\n".format(envname))
        out.write("channels:\n")
        for channel in channels:
            out.write("  - {}\n".format(channel))
        out.write("dependencies:\n")

        for line in history:
            if line.startswith('+'):
                # Replace the first two '-' from the right with '='
                package = '='.join(line.split('::')[-1].rsplit('-', 2))
                out.write("  - {}".format(package))


if __name__ == '__main__':
    args = parser.parse_args()

    channels = [args.channels]
    if "," in args.channels:
        channels = args.channels.split(",")
    # extra = [args.extra]
    # if "," in args.extra:
    #     extra = args.extra.split(",")

    main(condadir=args.dir,
         envfile=args.env_file,
         envname=args.env_name,
         channels=channels)
