# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc
from .. import _utils as su
from .tools import make_fake_coord


def plot(scipp_obj,
         interactive=True,
         projection=None,
         axes=None,
         color=None,
         marker=None,
         linestyle=None,
         linewidth=None,
         bins=None,
         is_doc_build=False,
         **kwargs):
    """
    Wrapper function to plot any kind of scipp object.
    """

    # Delayed imports
    from .tools import get_line_param
    from .dispatch import dispatch
    import matplotlib.pyplot as plt
    import matplotlib as mpl

    # Determine if we are using a static or interactive backend
    interactive = mpl.get_backend().lower().endswith('nbagg')

    # Switch interactive plotting off for better control over when figures are
    # displayed.
    # TODO: we need to think about what users expect, if they than have custom
    # figures further down in the notebook. We would want to switch
    # interactivity back on, but then we would have to turn it off again every
    # time a SciPlot slider is moved or a button is pressed.
    plt.ioff()

    inventory = dict()
    if su.is_dataset(scipp_obj):
        for name in sorted(scipp_obj.keys()):
            inventory[name] = scipp_obj[name]
    elif su.is_variable(scipp_obj):
        coords = {}
        for dim, size in zip(scipp_obj.dims, scipp_obj.shape):
            coords[dim] = make_fake_coord(dim, size)
        inventory[str(type(scipp_obj))] = sc.DataArray(data=scipp_obj,
                                                       coords=coords)
    elif su.is_data_array(scipp_obj):
        inventory[scipp_obj.name] = scipp_obj
    elif isinstance(scipp_obj, dict):
        inventory = scipp_obj
    else:
        raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                           "a Dataset, a DataArray, a Variable (and their "
                           "respective proxies), and a dict of "
                           "DataArrays.".format(type(scipp_obj)))

    # Prepare container for matplotlib line parameters
    line_params = {
        "color": color,
        "marker": marker,
        "linestyle": linestyle,
        "linewidth": linewidth
    }

    # Counter for 1d/event data
    line_count = -1

    # Create a list of variables which will then be dispatched to correct
    # plotting function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinates and units.
    # tobeplotted is a dict that holds four items:
    # {number_of_dimensions, Dataset, axes, line_parameters}.
    tobeplotted = dict()
    for name, var in sorted(inventory.items()):

        if sc.contains_events(var) and bins is None:
            raise RuntimeError("The `bins` argument must be specified when "
                               "plotting event data.")

        ndims = len(var.dims)
        if bins is not None and sc.contains_events(var):
            ndims += 1
        if ndims > 0:
            # ax = axes
            if ndims == 1 or projection == "1d" or projection == "1D":
                # Construct a key from the dimensions
                if axes is not None:
                    # # Check if we are dealing with a dict mapping dimensions to
                    # # labels
                    # if isinstance(axes, dict):
                    #     key = axes[str(var.dims[0])]
                    #     ax = [key]
                    # else:
                    #     key = ".".join(axes)
                    key = str(list(axes.values())[0])
                else:
                    key = str(var.dims[0])
                # Add unit to key
                key = "{}.{}".format(key, str(var.unit))
                line_count += 1
            else:
                key = name

            mpl_line_params = {}
            for n, p in line_params.items():
                if p is None:
                    mpl_line_params[n] = line_count
                elif isinstance(p, list):
                    mpl_line_params[n] = p[line_count]
                elif isinstance(p, dict):
                    if name in p:
                        mpl_line_params[n] = p[name]
                    else:
                        mpl_line_params[n] = line_count
                else:
                    mpl_line_params[n] = p
                if isinstance(mpl_line_params[n], int):
                    mpl_line_params[n] = get_line_param(
                        name=n, index=mpl_line_params[n])

            if key not in tobeplotted.keys():
                tobeplotted[key] = dict(ndims=ndims,
                                        scipp_obj_dict=dict(),
                                        axes=axes,
                                        mpl_line_params=dict())
                for n in mpl_line_params.keys():
                    tobeplotted[key]["mpl_line_params"][n] = {}
            tobeplotted[key]["scipp_obj_dict"][name] = inventory[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n][name] = p

    # Plot all the subsets
    output = Plot()
    for key, val in tobeplotted.items():
        sciplot = dispatch(scipp_obj_dict=val["scipp_obj_dict"],
                           name=key,
                           ndim=val["ndims"],
                           projection=projection,
                           axes=val["axes"],
                           mpl_line_params=val["mpl_line_params"],
                           bins=bins,
                           **kwargs)
        if not interactive:
            sciplot.as_static(keep_widgets=is_doc_build)
        output[key] = sciplot

    plt.ion()
    return output


class Plot(dict):
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an ipython repr.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    More functionalities can be added in the future.
    """
    def __init__(self, *arg, **kw):
        super(Plot, self).__init__(*arg, **kw)

    def _ipython_display_(self):
        import ipywidgets as widgets
        contents = []
        for key, val in self.items():
            contents.append(val._to_widget())
        return widgets.VBox(contents)._ipython_display_()
