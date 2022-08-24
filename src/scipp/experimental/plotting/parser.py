# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)




def _extract_args_from_list(selected, kwargs, name):
    out = {}
    for key, value in kwargs.items():
        if key in selected:
            if isinstance(value, dict):
                if name in value:
                    out[key] = value[name]
            else:
                out[key] = value
    return out


def parse_line_args(kwargs, name):
    # line_args = {}
    # if 'color' in kwargs:
    #     line_args['color'] = kwargs['color']
    # if 'linewidth' in kwargs:
    #     line_args['linewidth'] = kwargs['linewidth']
    # if 'linestyle' in kwargs:
    #     line_args['linestyle'] = kwargs['linestyle']
    # if 'marker' in kwargs:
    #     line_args['marker'] = kwargs['marker']
    # return line_args
    line_args = ('color', 'linewidth', 'linestyle', 'marker', 'mask_color', 'errorbars')
    # return {key: value for key, value in kwargs.items() if key in line_args}
    return _extract_args_from_list(line_args, kwargs, name)


def parse_mesh_args(kwargs, name):
    # mesh_args = {}
    # if 'aspect' in kwargs:
    #     mesh_args['aspect'] = kwargs['aspect']
    # if 'cmap' in kwargs:
    #     mesh_args['cmap'] = kwargs['cmap']
    # if 'cbar' in kwargs:
    #     mesh_args['cbar'] = kwargs['cbar']
    # return mesh_args
    mesh_args = ('cmap', 'cbar')
    # return {key: value for key, value in kwargs.items() if key in mesh_args}
    return _extract_args_from_list(mesh_args, kwargs, name)


def get_line_param(name: str, index: int) -> Any:
    """
    Get the default line parameter from the config.
    If an index is supplied, return the i-th item in the list.
    """
    param = config['plot'][name]
    return param[index % len(param)]


def get_cmap(name: str):

    if name is None:
        name = config['plot']['params']['cmap']

    try:
        cmap = copy(cm.get_cmap(name))
    except ValueError:
        cmap = LinearSegmentedColormap.from_list("tmp", [name, name])

    cmap.set_under(config['plot']['params']["under_color"])
    cmap.set_over(config['plot']['params']["over_color"])
    return cmap

def parse_step_kwargs(kwargs):



def _make_line_params(number, **kwargs):
    params = {}
    for key, arg in kwargs.items():
        if isinstance(arg, dict):
            if data.name in arg:
                params[key] = arg[data.name]
        else:
            params[key] = arg
        if params[key] is None:
            params[key] = get_line_param(key, number)
    return params


# def parse_args(**kwargs):

#     return {'line': _parse_line_args(**kwargs), 'mesh': _parse_mesh_args(**kwargs)}
