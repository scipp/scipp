# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev

from __future__ import annotations

from ..core import DataGroup, Variable
from ..typing import VariableLike


def make_html(container: VariableLike) -> str:
    """Return the HTML representation of an object.

    See 'HTML representation' in
    `Representations and Tables <../../user-guide/representations-and-tables.rst>`_
    for details.

    Parameters
    ----------
    container:
        Object to render.

    Returns
    -------
    :
        HTML representation

    See Also
    --------
    scipp.to_html:
        Display the HTML representation.
    """
    from .formatting_datagroup_html import datagroup_repr
    from .formatting_html import data_array_dataset_repr, variable_repr

    if isinstance(container, Variable):
        return variable_repr(container)
    elif isinstance(container, DataGroup):
        return datagroup_repr(container)
    else:
        return data_array_dataset_repr(container)


def to_html(container: VariableLike) -> None:
    """Render am object to HTML in a Jupyter notebook.

    Parameters
    ----------
    container:
        Object to render.

    See Also
    --------
    scipp.make_html:
        Create HTML representation without showing it.
        Works outside of notebooks.
    """
    from IPython.display import HTML, display

    display(HTML(make_html(container)))  # type: ignore[no-untyped-call]
