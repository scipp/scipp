# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen


def running_in_jupyter() -> bool:
    """
    Detect whether Python is running in Jupyter.

    Note that this includes not only Jupyter notebooks
    but also Jupyter console and qtconsole.
    """
    try:
        import ipykernel.zmqshell
        from IPython import get_ipython  # type: ignore[attr-defined]
    except ImportError:
        # Cannot be Jupyter if IPython is not installed.
        return False

    return isinstance(
        get_ipython(),  # type: ignore[no-untyped-call]
        ipykernel.zmqshell.ZMQInteractiveShell,
    )
