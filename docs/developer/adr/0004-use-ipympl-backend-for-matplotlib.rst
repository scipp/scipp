ADR 0004: Use the ipympl backend for Matplotlib figures in Jupyter
==================================================================

- Status: accepted
- Deciders: Neil, Simon
- Date: October 2020

Context
-------

Arranging widgets around Matplotlib figures when using the ``notebook`` backend in Jupyter was difficult.
The plot is displayed as soon as it is created, and the widgets can only be shown below a plot.
In addition, the ``notebook`` backend only works in the classic notebook, and not in JupyterLab.

Decision
--------

We switched to using the ``ipympl`` Matplotlib backend, where figures are now also ``ipywidgets`` and can be ordered in containers alongside other widgets.

Consequences
------------

Positive:
~~~~~~~~~

- Unifies the matplotlib figures with the slider widgets and the ``pythreejs`` scene for 3d plots.
- Works in JupyterLab.

Negative:
~~~~~~~~~

- Adds a new dependency, as the ``ipympl`` backend does not ship with Jupyter by default.
