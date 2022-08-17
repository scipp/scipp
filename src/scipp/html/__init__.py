# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev

from __future__ import annotations

from ..typing import VariableLike
from ..core import Variable
from .table import table  # noqa: F401


def make_html(container: VariableLike) -> str:
    """Return the HTML representation of an object.

    See 'HTML representation' in
    `Representations and Tables <../../visualization/representations-and-tables.rst>`_
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
    from .formatting_html import dataset_repr, variable_repr
    if isinstance(container, Variable):
        return variable_repr(container)
    else:
        return dataset_repr(container)


def to_html(container: VariableLike):
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
    from IPython.display import display, HTML
    display(HTML(make_html(container)))
