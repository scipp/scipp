# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
"""
Benchmark compilation in various different configurations.
See section 'CONFIGURATION' below for setup.
"""

import re
import subprocess
import sys
from dataclasses import dataclass, field
from datetime import timedelta
from itertools import chain, groupby
from pathlib import Path
from tempfile import TemporaryDirectory


@dataclass
class Case:
    name: str
    cmake_args: dict[str, str] = field(default_factory=dict)
    build_args: list[str] = field(default_factory=list)


##############################
#      CONFIGURATION         #

# Source directory, defaults to root of scipp repository.
SRCDIR = None

# Build target for CMake.
TARGET = "install"

# CMake configuration arguments used for every case.
COMMON_CMAKE_ARGS = {
    "CMAKE_CXX_COMPILER": None,
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_INTERPROCEDURAL_OPTIMIZATION": "OFF",
    "DYNAMIC_LIB": "ON",
}

# Build arguments used for every case.
COMMON_BUILD_ARGS = ['-j6']

# Cases to benchmark, each can specify a set of options
# for configuration and build.
CASES = [
    Case(name='base'),
    Case(name='feature', cmake_args={"MY_FEATURE": 'ON'}),
    Case(name='serial', build_args=['-j1']),
]

# For every set of files provided here, the build is re-run after touching
# those files. A clean build is always performed to set the baseline
# and configure the setup.
TOUCH = [
    [Path('variable/include/scipp/variable/transform.h')],
    [Path('core/include/scipp/core/multi_index.h')],
]

#                            #
##############################

# apply default config
SRCDIR = Path(Path(__file__).resolve().parent.parent if SRCDIR is None else SRCDIR)


def make_dirs(base_dir):
    base_dir = Path(base_dir).resolve()
    index = 0
    while True:
        build_dir = base_dir / f'build_{index:04d}'
        build_dir.mkdir()
        install_dir = base_dir / f'install_{index:04d}'
        install_dir.mkdir()
        index += 1
        yield build_dir, install_dir


def grope(paths):
    if paths is None:
        return
    for path in paths:
        (SRCDIR / path).touch()


def configure(case, build_dir, install_dir):
    cmake_args = [
        f'-D{key}={val}'
        for key, val in chain(COMMON_CMAKE_ARGS.items(), case.cmake_args.items())
        if val
    ]
    cmake_args.extend(
        [
            f'-DCMAKE_INSTALL_PREFIX={install_dir}',
            f'-DPython_EXECUTABLE={sys.executable}',
            str(SRCDIR),
        ]
    )

    try:
        subprocess.run(  # noqa: S603
            ['cmake', *cmake_args],  # noqa:  S607
            capture_output=True,
            check=True,
            encoding='utf-8',
            cwd=build_dir,
        )
    except subprocess.CalledProcessError as err:
        print(f"Configuring build '{case.name}' failed:\n{err.stdout}\n{err.stderr}")
        sys.exit(1)


@dataclass
class Times:
    real: timedelta
    user: timedelta

    @classmethod
    def parse(cls, msg):
        time_pattern = re.compile(r'(real|user|sys)\s+(\d+)m(\d+)[,.]?(\d*)s')
        times_dict = {}
        for line in msg.rsplit('\n', 10)[
            1:
        ]:  # do not look at all of the output, it can get very long
            match = time_pattern.match(line)
            if match:
                # Truncate sub second results, those will not be reliable.
                times_dict[match[1]] = timedelta(
                    minutes=int(match[2]), seconds=int(match[3])
                )
        try:
            return cls(times_dict['real'], times_dict['user'])
        except KeyError:
            print('Did not find all required time measurements in the output')
            raise


def build(case, build_dir):
    build_args = ['--build', '.', '--target', TARGET]
    all_build_args = [*COMMON_BUILD_ARGS, *case.build_args]
    if all_build_args:
        build_args.append('--')
        build_args.extend(all_build_args)
    try:
        res = subprocess.run(  # noqa: S602
            ' '.join(['time', 'cmake', *build_args]),
            capture_output=True,
            check=True,
            encoding='utf-8',
            cwd=build_dir,
            shell=True,
        )
    except subprocess.CalledProcessError as err:
        print(f"Build '{case.name}' failed:\n{err.stdout}\n{err.stderr}")
        sys.exit(1)

    return Times.parse(res.stderr)


def report(results):
    print('Result:\n')
    results = sorted(
        sorted(results, key=lambda t: t[0]),
        key=lambda t: ' '.join('' if t[1] == 'clean' else str(t[1])),
    )
    printed_names = False
    for touch, group in groupby(results, key=lambda t: t[1]):
        names, _, times = zip(*group, strict=True)
        if not printed_names:
            print(
                '                      '
                + '  '.join(f'{name:20s}' for name in names)
                + '\n'
            )
            printed_names = True
        if touch:
            for path in touch[:-1]:
                print(f'{str(path)[-20::]:20s}')
        touch = touch[-1] if touch else ''
        time_strs = [
            f'R{time.real.seconds // 60:2d}m{time.real.seconds % 60:02d}s '
            f'U{time.user.seconds // 60:2d}m{time.user.seconds % 60:02d}s'
            for time in times
        ]
        print(
            f'{str(touch)[-20::]:20s}  '
            + '  '.join(f'{time:20s}' for time in time_strs)
            + '\n'
        )


def main():
    results = []
    with TemporaryDirectory(dir='./') as working_dir:
        for case, dirs in zip(CASES, make_dirs(working_dir), strict=True):
            print(f"Running '{case.name}'")
            print('    Clean build')
            configure(case, *dirs)
            times = build(case, dirs[0])
            results.append((case.name, ['clean'], times))

            for touch in TOUCH:
                grope(touch)
                print('    Touching ', touch)
                times = build(case, dirs[0])
                results.append((case.name, touch, times))

    report(results)


if __name__ == '__main__':
    main()
