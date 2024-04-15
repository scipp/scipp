# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

# Original source from
# https://github.com/jsignell/xarray/blob/1d960933ab252e0f79f7e050e6c9261d55568057/xarray/core/formatting_html.py

import collections
import operator
import uuid
from functools import partial, reduce
from html import escape as html_escape
from re import escape as re_escape

from .._scipp import core as sc
from ..core import stddevs
from ..utils import value_to_string
from .resources import load_icons, load_style

BIN_EDGE_LABEL = "[bin-edge]"
STDDEV_PREFIX = "σ = "  # noqa: RUF001
VARIANCES_SYMBOL = "σ²"
SPARSE_PREFIX = "len={}"


def escape(content: str) -> str:
    """
    Escape dollar-sign($) as well as html special characters.
    Dollar-sign($) without any escape causes unexpected MathJax conversion
    in the Jupyter notebook.
    """
    return html_escape(content).replace('$', re_escape('$'))


def _format_array(data, size, ellipsis_after):
    i = 0
    s = []
    while i < size:
        if i == ellipsis_after and size > 2 * ellipsis_after + 1:
            s.append("...")
            i = size - ellipsis_after
        elem = data[i]
        if isinstance(elem, sc.DataArray):
            dims = ', '.join(f'{dim}: {s}' for dim, s in elem.sizes.items())
            coords = ', '.join(elem.coords)
            if elem.unit == sc.units.one:
                s.append(f'{{dims=[{dims}], coords=[{coords}]}}')
            else:
                s.append(f'{{dims=[{dims}], unit={elem.unit}, coords=[{coords}]}}')
        else:
            s.append(value_to_string(elem))
        i += 1
    return escape(", ".join(s))


def _make_row(data_html):
    return f"<div>{data_html}</div>"


def _format_non_events(var, has_variances):
    size = reduce(operator.mul, var.shape, 1)
    if len(var.dims):
        var = sc.flatten(var, var.dims, 'ignored')
    if has_variances:
        data = stddevs(var).values
    else:
        data = var.values
    # avoid unintentional indexing into value of 0-D data
    if len(var.shape) == 0:
        data = [data]
    s = _format_array(data, size, ellipsis_after=2)
    if has_variances:
        s = f'{STDDEV_PREFIX}{s}'
    return s


def _repr_item(bin_dim, item):
    shape = item.shape[bin_dim]
    return SPARSE_PREFIX.format(shape)


def _get_events(var, variances, ellipsis_after):
    dim = var.bins.constituents['dim']
    dims = var.bins.constituents['data'].dims
    bin_dim = dict(zip(dims, range(len(dims))))[dim]
    s = []
    if not isinstance(var.values, (sc.Variable, sc.DataArray, sc.Dataset)):
        size = len(var.values)
        i = 0

        data = retrieve(var, variances=variances)
        while i < size:
            if i == ellipsis_after and size > 2 * ellipsis_after + 1:
                s.append("...")
                i = size - ellipsis_after
            item = data[i]
            s.append(_repr_item(bin_dim, item))
            i += 1
    else:
        s.append(
            _repr_item(
                bin_dim,
                var.value,
            )
        )
    return s


def _format_events(var, has_variances):
    s = _get_events(var, has_variances, ellipsis_after=2)
    return f'binned data [{", ".join([row for row in s])}]'


def _ordered_dict(data):
    data_ordered = collections.OrderedDict(
        sorted(data.items(), key=lambda t: str(t[0]))
    )
    return data_ordered


def inline_variable_repr(var, has_variances=False):
    if var.bins is None:
        if isinstance(var, sc.DataArray):
            return _format_non_events(var.data, has_variances)
        else:
            return _format_non_events(var, has_variances)
    else:
        return _format_events(var, has_variances)


def retrieve(var, variances=False, single=False):
    if not variances:
        return var.value if single else var.values
    else:
        return var.variance if single else var.variances


def _short_data_repr_html_non_events(var, variances=False):
    return repr(retrieve(var, variances))


def _short_data_repr_html_events(var):
    string = str(var.data) if isinstance(var, sc.DataArray) else str(var)
    if isinstance(var.bins.constituents['data'], sc.Dataset):
        return string
    start = 'binned data: '
    ind = string.find(start) + len(start)
    return string[ind:].replace(', content=', ',\ncontent=')


def short_data_repr_html(var, variances=False):
    """Format "data" for DataArray and Variable."""
    if var.bins is not None:
        data_repr = _short_data_repr_html_events(var)
    else:
        data_repr = _short_data_repr_html_non_events(var, variances)
    return escape(data_repr)


def format_dims(dims, sizes, coords):
    if not dims:
        return ""

    dim_css_map = {
        dim: " class='sc-has-index'" if dim in coords else "" for dim in dims
    }

    dims_li = "".join(
        f"<li><span{dim_css_map[dim]}>"
        f"{escape(str(dim))}</span>: "
        f"{size if size is not None else 'Events' }</li>"
        for dim, size in zip(dims, sizes)
    )

    return f"<ul class='sc-dim-list'>{dims_li}</ul>"


def _icon(icon_name):
    # icon_name is defined in icon-svg-inline.html
    return (
        "<svg class='icon sc-{0}'>" "<use xlink:href='#{0}'>" "</use>" "</svg>".format(
            icon_name
        )
    )


def summarize_coord(dim, var, ds=None):
    return summarize_variable(str(dim), var, is_aligned=var.aligned, embedded_in=ds)


def summarize_mask(dim, var, ds=None):
    return summarize_variable(str(dim), var, is_aligned=False, embedded_in=ds)


def summarize_coords(coords, ds=None):
    vars_li = "".join(
        "<li class='sc-var-item'>" f"{summarize_coord(dim, var, ds)}" "</span></li>"
        for dim, var in _ordered_dict(coords).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def summarize_masks(masks, ds=None):
    vars_li = "".join(
        "<li class='sc-var-item'>" f"{summarize_mask(dim, var, ds)}" "</span></li>"
        for dim, var in _ordered_dict(masks).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def summarize_attrs(attrs, embedded_in=None):
    attrs_li = "".join(
        "<li class='sc-var-item'>{}</li>".format(
            summarize_variable(
                name,
                var,
                has_attrs=False,
                embedded_in=embedded_in,
                is_aligned=False,
            )
        )
        for name, var in _ordered_dict(attrs).items()
    )
    return f"<ul class='sc-var-list'>{attrs_li}</ul>"


def _find_bin_edges(var, ds):
    """
    Checks if the coordinate contains bin-edges.
    """
    return [
        dim for dim, length in var.sizes.items() if ds.sizes.get(dim, 1) + 1 == length
    ]


def _make_inline_attributes(var, has_attrs, embedded_in):
    disabled = "disabled"
    attrs_ul = ""
    attrs_sections = []

    if has_attrs and hasattr(var, "masks"):
        if len(var.masks) > 0:
            attrs_sections.append(mask_section(var.masks))
            disabled = ""

    if has_attrs and hasattr(var, "deprecated_attrs"):
        if len(var.attrs) > 0:
            attrs_sections.append(attr_section(var.deprecated_attrs, embedded_in))
            disabled = ""

    if len(attrs_sections) > 0:
        attrs_sections = "".join(
            f"<li class='sc-section-item sc-subsection'>{s}</li>"
            for s in attrs_sections
        )
        attrs_ul = (
            "<div class='sc-wrap'>"
            f"<ul class='sc-sections'>{attrs_sections}</ul>"
            "</div>"
        )

    return disabled, attrs_ul


def _make_dim_labels(dim, bin_edges=None):
    # Note: the space needs to be here, otherwise
    # there is a trailing whitespace when no dimension
    # label has been added
    if bin_edges and dim in bin_edges:
        return f" {BIN_EDGE_LABEL}"
    else:
        return ""


def _make_dim_str(var, bin_edges, add_dim_size=False):
    dims_text = ', '.join(
        '{}{}{}'.format(
            str(dim),
            _make_dim_labels(dim, bin_edges),
            f': {size}' if add_dim_size and size is not None else '',
        )
        for dim, size in zip(var.dims, var.shape)
    )
    return dims_text


def _format_common(is_index):
    cssclass_aligned = " class='sc-aligned'" if is_index else ""

    # "unique" ids required to expand/collapse subsections
    attrs_id = "attrs-" + str(uuid.uuid4())
    data_id = "data-" + str(uuid.uuid4())
    attrs_icon = _icon("icon-file-text2")
    data_icon = _icon("icon-database")

    return cssclass_aligned, attrs_id, attrs_icon, data_id, data_icon


def summarize_variable(
    name, var, is_aligned=False, has_attrs=False, embedded_in=None, add_dim_size=False
):
    """
    Formats the variable data into the format expected when displaying
    as a standalone variable (when a single variable or data array is
    displayed) or as part of a dataset.
    """
    dims_str = "({})".format(
        _make_dim_str(
            var,
            _find_bin_edges(var, embedded_in) if embedded_in is not None else None,
            add_dim_size,
        )
    )
    if var.unit is None:
        unit = ''
    else:
        unit = '𝟙' if var.unit == sc.units.dimensionless else str(var.unit)  # noqa: RUF001

    disabled, attrs_ul = _make_inline_attributes(var, has_attrs, embedded_in)

    preview = _make_row(inline_variable_repr(var))
    data_repr = short_data_repr_html(var)
    if var.bins is None:
        data_repr = "Values:<br>" + data_repr
    variances_preview = None
    if var.variances is not None:
        variances_preview = _make_row(inline_variable_repr(var, has_variances=True))
        data_repr += f"<br><br>Variances ({VARIANCES_SYMBOL}):<br>\
{short_data_repr_html(var, variances=True)}"

    cssclass_aligned, attrs_id, attrs_icon, data_id, data_icon = _format_common(
        is_aligned
    )

    if name is None:
        html = [
            f"<div class='sc-standalone-var-name'><span{cssclass_aligned}>"
            f"{escape(dims_str)}</span></div>"
        ]
    else:
        html = [
            f"<div class='sc-var-name'><span{cssclass_aligned}>{escape(str(name))}"
            "</span></div>",
            f"<div class='sc-var-dims'>{escape(dims_str)}</div>",
        ]
    html += [
        f"<div class='sc-var-dtype'>{escape(str(var.dtype))}</div>",
        f"<div class='sc-var-unit'>{escape(unit)}</div>",
        f"<div class='sc-value-preview sc-preview'><span>{preview}</span>",
        "{}</div>".format(
            f'<span>{variances_preview}</span>' if variances_preview is not None else ''
        ),
        f"<input id='{attrs_id}' class='sc-var-attrs-in' ",
        f"type='checkbox' {disabled}>",
        f"<label for='{attrs_id}' "
        f"class='{'' if has_attrs else 'sc-hide-icon'}'"
        " title='Show/Hide attributes'>",
        f"{attrs_icon}</label>",
        f"<input id='{data_id}' class='sc-var-data-in' type='checkbox'>",
        f"<label for='{data_id}' title='Show/Hide data repr'>",
        f"{data_icon}</label>",
        f"<div class='sc-var-attrs'>{attrs_ul}</div>" if attrs_ul else "",
        f"<pre class='sc-var-data'>{data_repr}</pre>",
    ]
    return "".join(html)


def summarize_data(dataset):
    has_attrs = isinstance(dataset, sc.Dataset)
    vars_li = "".join(
        "<li class='sc-var-item'>{}</li>".format(
            summarize_variable(
                name,
                var.data,
                has_attrs=has_attrs,
                embedded_in=dataset if has_attrs else None,
            )
        )
        for name, var in _ordered_dict(dataset).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def collapsible_section(
    name, inline_details="", details="", n_items=None, enabled=True, collapsed=False
):
    # "unique" id to expand/collapse the section
    data_id = "section-" + str(uuid.uuid4())

    has_items = n_items is not None and n_items
    n_items_span = "" if n_items is None else f" <span>({n_items})</span>"
    enabled = "" if enabled and has_items else "disabled"
    collapsed = "" if collapsed or not has_items else "checked"
    tip = " title='Expand/collapse section'" if enabled else ""

    return (
        f"<input id='{data_id}' class='sc-section-summary-in' "
        f"type='checkbox' {enabled} {collapsed}>"
        f"<label for='{data_id}' class='sc-section-summary' {tip}>"
        f"{name}:{n_items_span}</label>"
        f"<div class='sc-section-inline-details'>{inline_details}</div>"
        f"<div class='sc-section-details'>{details}</div>"
    )


def _mapping_section(
    mapping,
    *extra_details_func_args,
    name=None,
    details_func=None,
    max_items_collapse=None,
    enabled=True,
):
    n_items = 1 if isinstance(mapping, sc.DataArray) else len(mapping)
    collapsed = n_items >= max_items_collapse

    return collapsible_section(
        name,
        details=details_func(mapping, *extra_details_func_args),
        n_items=n_items,
        enabled=enabled,
        collapsed=collapsed,
    )


def dim_section(dataset):
    coords = dataset.coords if hasattr(dataset, "coords") else dict()
    dim_list = format_dims(dataset.dims, dataset.shape, coords)

    return collapsible_section(
        "Dimensions", inline_details=dim_list, enabled=False, collapsed=True
    )


def summarize_array(var, is_variable=False):
    vars_li = "".join(
        "<li class='sc-var-item'>"
        f"{summarize_variable(None, var, add_dim_size=is_variable)}</li>"
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def variable_section(var):
    return summarize_array(var, is_variable=True)


coord_section = partial(
    _mapping_section,
    name="Coordinates",
    details_func=summarize_coords,
    max_items_collapse=25,
)

mask_section = partial(
    _mapping_section, name="Masks", details_func=summarize_masks, max_items_collapse=10
)

data_section = partial(
    _mapping_section,
    name="Data",
    details_func=summarize_data,
    max_items_collapse=15,
)

attr_section = partial(
    _mapping_section,
    name="Attributes",
    details_func=summarize_attrs,
    max_items_collapse=10,
)


def _obj_repr(header_components, sections):
    header = f"<div class='sc-header'>" f"{''.join(h for h in header_components)}</div>"
    sections = "".join(f"<li class='sc-section-item'>{s}</li>" for s in sections)

    return (
        "<div>"
        f"{load_icons()}"
        f"{load_style()}"
        "<div class='sc-wrap sc-root'>"
        f"{header}"
        f"<ul class='sc-sections'>{sections}</ul>"
        "</div>"
        "</div>"
    )


def _format_size(obj):
    view_size = obj.__sizeof__()
    underlying_size = obj.underlying_size()
    res = f"({human_readable_size(view_size)}"
    if view_size != underlying_size:
        res += (
            " <span class='sc-underlying-size'>out of "
            f"{human_readable_size(underlying_size)}</span>"
        )
    return res + ")"


def dataset_repr(ds):
    obj_type = "scipp.{}".format(type(ds).__name__)
    header_components = [
        f"<div class='sc-obj-type'>{escape(obj_type)} " + _format_size(ds) + "</div>"
    ]

    sections = [dim_section(ds)]

    if len(ds.coords) > 0:
        sections.append(coord_section(ds.coords, ds))

    sections.append(data_section(ds if isinstance(ds, sc.Dataset) else {'': ds}))

    if not isinstance(ds, sc.Dataset):
        if len(ds.masks) > 0:
            sections.append(mask_section(ds.masks, ds))
        if len(ds.deprecated_attrs) > 0:
            sections.append(attr_section(ds.deprecated_attrs, ds))

    return _obj_repr(header_components, sections)


def variable_repr(var):
    obj_type = "scipp.{}".format(type(var).__name__)

    header_components = [
        f"<div class='sc-obj-type'>{escape(obj_type)} " + _format_size(var) + "</div>"
    ]

    sections = [variable_section(var)]

    return _obj_repr(header_components, sections)


def human_readable_size(size_in_bytes):
    if size_in_bytes / (1024 * 1024 * 1024) > 1:
        return f'{size_in_bytes/(1024*1024*1024):.2f} GB'
    if size_in_bytes / (1024 * 1024) > 1:
        return f'{size_in_bytes/(1024*1024):.2f} MB'
    if size_in_bytes / (1024) > 1:
        return f'{size_in_bytes/(1024):.2f} KB'

    return f'{size_in_bytes} Bytes'
