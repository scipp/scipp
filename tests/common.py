# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


def assert_export(f, *args, **kwargs):
    """
    Checks that a function is exported via pybind11 and is callable with
    specified arguments.

    CONSIDER FOR REMOVAL because it does nothing but call f.
    """
    f(*args, **kwargs)
