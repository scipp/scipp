# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .tools import is_static

_backend = 'stable'


def plot_stable(*args, **kwargs):
    """
    Plot a Scipp object.

    Possible inputs are:
      - Variable
      - Dataset
      - DataArray
      - numpy ndarray
      - dict of Variables
      - dict of DataArrays
      - dict of numpy ndarrays
      - dict that can be converted to a Scipp object via `from_dict`

    For more details, see
    https://scipp.github.io/visualization/plotting-overview.html

    :param aspect: Specify the aspect ratio for 2d images.
         Possible values are `"auto"` or `"equal"`.
         Defaults to `"auto"`.
    :type aspect: str, optional

    :param ax: Attach returned plot to supplied Matplotlib axes (1d and 2d
        only). Defaults to `None`.
    :type ax: matplotlib.axes.Axes, optional

    :param labels: Dict specifying which coordinate should be used to label
        the tics for a dimension. If not specifified the dimension coordinate
        is used. `labels={"time": "time-labels"}`. Defaults to `None`.
    :type labels: dict, optional

    :param camera: Dict configuring the camera. Valid entries are 'position' and
        'look_at'. This option is valid only for 3-D scatter plots.
        The 'position' entry defines the position of the camera and the 'look_at'
        entry defines the point the camera is looking at.
        Both must be variables containing a single vector with the correct unit,
        i.e., a unit compatible with the unit of the scatter point positions.
        Defaults to `None`, in which case the camera looks at the center of the
        cloud of plotted points.
    :type camera: dict, optional

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

    :param resampling_mode: Resampling mode. Possible choices are `"sum"` and
        `"mean"`. This applies only to binned event data and non-1d data.
        Defaults to `"mean"` unless the unit is 'counts' or 'dimensionless'.
    :type resampling_mode: str, optional

    :param scale: Specify the scale (`"linear"` or `"log"`) for a displayed
        dimension axis. E.g. `scale={"tof": "log"}`. Defaults to None.
    :type scale: dict, optional

    :param vmin: Minimum value for the y-axis (1d) or colorscale (2d and 3d).
        Defaults to None.
    :type vmin: float, optional

    :param vmax: Maximum value for the y-axis (1d) or colorscale (2d and 3d).
        Defaults to None.
    :type vmax: float, optional

    """
    import matplotlib.pyplot as plt
    from .plot import plot as _plot

    # Switch auto figure display off for better control over when figures are
    # displayed.
    interactive_on = False
    if plt.isinteractive():
        plt.ioff()
        interactive_on = True

    output = _plot(*args, **kwargs)

    if (output is not None) and is_static():
        output.hide_widgets()

    # Turn auto figure display back on if needed.
    if interactive_on:
        plt.ion()

    return output


def raise_bad_backend(backend: str):
    raise ValueError(f"Unknown plotting backend {backend}. "
                     "Possible choices are 'stable' and 'experimental'.")


def plot(*args, **kwargs):
    global _backend
    if _backend == 'stable':
        return plot_stable(*args, **kwargs)
    elif _backend == 'experimental':
        from ..experimental.plotting import plot as plot_experimental
        return plot_experimental(*args, **kwargs)
    else:
        raise_bad_backend(_backend)


def select_backend(new_backend: str):
    global _backend
    if new_backend == 'stable':
        plot.__doc__ = plot_stable.__doc__
    elif new_backend == 'experimental':
        from ..experimental.plotting import plot as plot_experimental
        plot.__doc__ = plot_experimental.__doc__
    else:
        raise_bad_backend(new_backend)
    _backend = new_backend


select_backend(_backend)
