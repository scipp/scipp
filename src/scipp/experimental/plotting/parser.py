# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


def _parse_line_args(**kwargs):
    line_args = {}
    if 'color' in kwargs:
        line_args['color'] = kwargs['color']
    if 'linewidth' in kwargs:
        line_args['linewidth'] = kwargs['linewidth']
    if 'linestyle' in kwargs:
        line_args['linestyle'] = kwargs['linestyle']
    if 'marker' in kwargs:
        line_args['marker'] = kwargs['marker']
    return line_args


def _parse_mesh_args(**kwargs):
    mesh_args = {}
    if 'aspect' in kwargs:
        mesh_args['aspect'] = kwargs['aspect']
    if 'cmap' in kwargs:
        mesh_args['cmap'] = kwargs['cmap']
    if 'cbar' in kwargs:
        mesh_args['cbar'] = kwargs['cbar']
    return mesh_args


def parse_args(**kwargs):

    return {'line': _parse_line_args(**kwargs), 'mesh': _parse_mesh_args(**kwargs)}

    out = obj
    if isinstance(out, np.ndarray):
        dims = [f"axis-{i}" for i in range(len(out.shape))]
        out = Variable(dims=dims, values=out)
    if isinstance(out, Variable):
        out = DataArray(data=out)
    for dim, size in out.sizes.items():
        if dim not in out.meta:
            out.coords[dim] = arange(dim, size)
    return out


def plot(obj: Union[VariableLike, Dict[str, VariableLike]], **kwargs) -> Figure:
    """Plot a Scipp object.

    Parameters
    ----------
    obj:
        The object to be plotted. Possible inputs are:
        - Variable
        - Dataset
        - DataArray
        - numpy ndarray
        - dict of Variables
        - dict of DataArrays
        - dict of numpy ndarrays

    Returns
    -------
    :
        A figure.
    """
    if isinstance(obj, (dict, Dataset)):
        to_plot = {key: _to_data_array(item) for key, item in obj.items()}
        nodes = [input_node(v) for v in to_plot.values()]
        return Figure(*nodes, **kwargs)
    else:
        return Figure(input_node(_to_data_array(obj)), **kwargs)
