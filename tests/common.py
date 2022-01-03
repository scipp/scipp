# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


def assert_export(f, *args, **kwargs):
    """
    Checks that a function is exported via pybind11 and is callable with
    specified arguments.
    Ignores any exceptions that the function may raise due to being passed
    garbage inputs.
    """
    try:
        f(*args, **kwargs)
    except TypeError as ex:
        if 'incompatible function arguments' in ex.args[0]:
            raise ex
        pass
    except:  # noqa: E722
        pass
