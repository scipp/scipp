# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .plot import plot as _plot
import warnings

is_doc_build = False

try:
    import matplotlib as mpl
except ImportError:
    mpl = None

try:
    from IPython import get_ipython
    ipy = get_ipython()
except ImportError:
    ipy = None

# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
if ipy is not None:
    # Check if a docs build is requested in the metadata. If so,
    # use the default Qt/inline backend.
    cfg = ipy.config
    meta = cfg["Session"]["metadata"]
    if hasattr(meta, "to_dict"):
        meta = meta.to_dict()
    if "scipp_docs_build" in meta:
        is_doc_build = meta["scipp_docs_build"]
    if ("IPKernelApp" in ipy.config) and (mpl is not None):
        try:
            # Attempt to use ipympl backend
            from ipympl.backend_nbagg import Canvas
            mpl.use('module://ipympl.backend_nbagg')
            # Hide the figure header:
            # see https://github.com/matplotlib/ipympl/issues/229
            Canvas.header_visible.default_value = False
        except ImportError:
            warnings.warn(
                "The ipympl backend, which is required for "
                "interactive plots in Jupyter, was not found. "
                "Falling back to a static backend. Use "
                "conda install -c conda-forge ipympl to install ipympl.")

# Note: due to some strange behaviour when importing matplotlib and pyplot in
# different order, we need to import pyplot after switching to the ipympl
# backend (see https://github.com/matplotlib/matplotlib/issues/19032).
try:
    import matplotlib.pyplot as plt
except ImportError:
    plt = None

if is_doc_build and plt is not None:
    plt.rcParams.update({'figure.max_open_warning': 0})


def plot(*args, **kwargs):
    """
    Plot a Scipp object.

    Possible inputs are:
    - Variable
    - DataArray
    - Dataset
    - dict of Variables
    - dict of DataArrays

    For more details, see
    https://scipp.github.io/visualization/plotting-overview.html

    :param aspect: Specify the aspect ratio for 2d images and 3d renderings.
         Possible values are `"auto"` or `"equal"`.
         Defaults to `"auto"`.
    :type aspect: str, optional

    :param ax: Attach returned plot to supplied Matplotlib axes (1d and 2d
        only). Defaults to `None`.
    :type ax: matplotlib.axes.Axes, optional

    :param axes: Specify which input dimension should be shown along which
        figure axis. E.g. to show the `"tof"` dimension along the vertical
        axis of a 2d image, use `axes={"y": "tof"}`.
        Defaults to `None`.
    :type axes: dict, optional

    :param bins: Specify on-the-fly binning when plotting event data.
        Possible values are:
        - an integer setting the number of bins
        - a `numpy` array setting the bin edges
        - a Variable setting the bin edges
        Defaults to `None`.
    :type bins: int or ndarray or Variable, optional

    :param cax: Attach colorbar to supplied Matplotlib axes.
        Defaults to `None`.
    :type cax: matplotlib.axes.Axes, optional

    :param cmap: Matplotlib colormap (2d and 3d only).
        See https://matplotlib.org/tutorials/colors/colormaps.html.
        Defaults to `None`.
    :type cmap: str, optional

    :param color: Matplotlib line color (1d only).
        See https://matplotlib.org/tutorials/colors/colors.html.
        Defaults to None.
    :type color: str, optional

    :param errorbars: Show errorbars if `True`, hide them if `False` (1d only).
        Defaults to `True`. This can also be a dict of `bool` where the keys
        correspond to data entries.
    :type errorbars: str or dict, optional

    :param figsize: The size of the figure in inches (1d and 2d only).
        See
        https://matplotlib.org/api/_as_gen/matplotlib.pyplot.figure.html.
        Defaults to `None`.
    :type figsize: tuple, optional

    :param filename: If specified, the figure will be saved to disk. Possible
        file extensions are `.jpg`, `.png` and `.pdf`. The default directory
        for writing the file is the same as the directory where the script or
        notebook is running. Defaults to `None`.
    :type filename: str, optional

    :param grid: Show grid on axes if `True`. Defaults to `False`.
    :type grid: bool, optional

    :param linestyle: Matplotlib linestyle (1d only).
        See
        https://matplotlib.org/gallery/lines_bars_and_markers/linestyles.html.
        Defaults to "none".
    :type linestyle: str, optional

    :param marker: Matplotlib line marker (1d only).
        See https://matplotlib.org/api/markers_api.html.
        Defaults to `'o'`.
    :type marker: str, optional

    :param masks: A dict to hold display parameters for masks such as a `color`
        or a `cmap`. Defaults to `None`.
    :type masks: dict, optional

    :param norm: Normalization of the data. Possible choices are `"linear"` and
        `"log"`. Defaults to `"linear"`.
    :type norm: str, optional

    :param pax: Attach profile plot to supplied Matplotlib axes.
        Defaults to `None`.
    :type pax: matplotlib.axes.Axes, optional

    :param pixel_size: Specify the size of the pixels to be used for the point
        cloud (3d only). If none is supplied, the size is guessed based on the
        extents of the data in the 3d space. Defaults to `None`.
    :type pixel_size: float, optional

    :param positions: Specify an array of position vectors to be used as
        scatter points positions (3d only). Defaults to `None`.
    :type positions: Variable, optional

    :param projection: Specify the projection to be used. Possible choices are
        `"1d"`, `"2d"`, or `"3d"`. Defaults to `"2d"` if the number of
        dimensions of the input is >= 2.
    :type projection: str, optional

    :param scale: Specify the scale (`"linear"` or `"log"`) for a displayed
        dimension axis. E.g. `scale={"tof": "log"}`. Defaults to None.
    :type scale: dict, optional

    :param vmin: Minimum value for the colorscale (2d and 3d only).
        Defaults to None.
    :type vmin: float, optional

    :param vmax: Maximum value for the colorscale (2d and 3d only).
        Defaults to None.
    :type vmax: float, optional

    """

    if plt is None:
        raise RuntimeError("Matplotlib not found. Matplotlib is required to "
                           "use plotting in Scipp.")

    # Switch auto figure display off for better control over when figures are
    # displayed.
    interactive_on = False
    if plt.isinteractive():
        plt.ioff()
        interactive_on = True

    output = _plot(*args, **kwargs)

    if output is not None:
        # Hide all widgets if this is the inline backend
        if mpl.get_backend().lower().endswith('inline'):
            output.hide_widgets()
        # Turn mpl figure into image if doc build
        if is_doc_build:
            output.make_static()

    # Turn auto figure display back on if needed.
    if interactive_on:
        plt.ion()

    return output


def superplot(*args, **kwargs):
    """
    Plot a Scipp object with a 1d projection that offers the possibility to
    keep individual profiles as coloured lines.
    """
    return plot(*args, projection="1d", **kwargs)


def image(*args, **kwargs):
    """
    Plot a Scipp object as a 2d image.
    """
    return plot(*args, projection="2d", **kwargs)


def scatter3d(*args, **kwargs):
    """
    Plot a Scipp object as a 3d scatter plot.
    """
    return plot(*args, projection="3d", **kwargs)
