# Original source from
# https://github.com/jsignell/xarray/blob/1d960933ab252e0f79f7e050e6c9261d55568057/xarray/core/formatting_html.py

import operator
import os
import uuid

from functools import partial, reduce
from html import escape
import numpy as np

from .._scipp import core as sc

CSS_FILE_PATH = f"{os.path.dirname(__file__)}/style.css"
with open(CSS_FILE_PATH, 'r') as f:
    CSS_STYLE = "".join(f.readlines())

ICONS_SVG_PATH = f"{os.path.dirname(__file__)}/icons-svg-inline.html"
with open(ICONS_SVG_PATH, 'r') as f:
    ICONS_SVG = "".join(f.readlines())


def _is_dataset(x):
    return isinstance(x, sc.Dataset) or isinstance(x, sc.DataProxy)


def _format_array(data, size, ellipsis_after, do_ellide=True):
    i = 0
    s = []
    while i < size:
        if do_ellide and i == ellipsis_after and size > 2 * ellipsis_after + 1:
            s.append("...")
            i = size - ellipsis_after
        elem = data[i]
        if not hasattr(data, "dtype") or data.dtype != np.bool:
            elem = round(elem, 2)
        s.append(str(elem))
        i += 1
    return escape(", ".join(s))


def _make_row(data_html, variances_html=None):
    return f"<div>{data_html}</div>"


def _format_non_sparse(var, has_variances):
    size = reduce(operator.mul, var.shape, 1)
    # flatten avoids displaying square brackets in the output
    data = retrieve(var, variances=has_variances).flatten()
    return _make_row(_format_array(data, size, ellipsis_after=2))


def _get_sparse(var, variances, ellipsis_after, summary=False):
    if hasattr(var, "data") and var.data is None:
        return ["no data, implicitly 1"]
    single = len(var.shape) == 0
    size = 1 if single else var.shape[0]
    i = 0
    s = []

    do_ellide = single or summary or size > 1000 or sum([
        len(retrieve(var, variances=variances)[i])
        for i in range(min(size, 1000))
    ]) > 1000

    data = retrieve(var, variances=variances)
    while i < size:
        if i == ellipsis_after and do_ellide and size > 2 * ellipsis_after + 1:
            s.append("...")
            i = size - ellipsis_after
        item = data if single else data[i]
        if summary:
            s.append(f'len={len(item)}')
        else:
            s.append('sparse({})'.format(
                _format_array(item, len(item), ellipsis_after, do_ellide)))
        i += 1
    return s


def _format_sparse(var, has_variances):
    s = _get_sparse(var, has_variances, ellipsis_after=1, summary=True)
    return _make_row(", ".join([row for row in s]))


def inline_variable_repr(var, has_variances=False):
    if var.sparse_dim is None:
        return _format_non_sparse(var, has_variances)
    else:
        return _format_sparse(var, has_variances)


def retrieve(var, variances=False, single=False):
    if not variances:
        return var.value if single else var.values
    else:
        return var.variance if single else var.variances


def _short_data_repr_html_non_sparse(var, variances=False):
    if hasattr(var, "data"):
        return repr(retrieve(var.data, variances))
    else:
        return repr(retrieve(var, variances))


def _short_data_repr_html_sparse(var, variances=False):
    return "array([" + ",\n       ".join(
        _get_sparse(var, variances, ellipsis_after=3)) + "])"


def short_data_repr_html(var, variances=False):
    """Format "data" for DataArray and Variable."""
    if var.dtype == sc.dtype.string:
        data_repr = str(retrieve(var, variances, single=True))
    else:
        if var.sparse_dim is None:
            data_repr = _short_data_repr_html_non_sparse(var, variances)
        else:
            data_repr = _short_data_repr_html_sparse(var, variances)
    return escape(data_repr)


def format_dims(dims, sizes, coords):
    if not dims:
        return ""

    dim_css_map = {
        dim: " class='xr-has-index'" if dim in coords else ""
        for dim in dims
    }

    dims_li = "".join(
        f"<li><span{dim_css_map[dim]}>"
        f"{escape(str(dim))}</span>: "
        f"{size if size != sc.Dimensions.Sparse else 'Sparse' }</li>"
        for dim, size in zip(dims, sizes))

    return f"<ul class='xr-dim-list'>{dims_li}</ul>"


def summarize_attrs_simple(attrs):
    attrs_dl = "".join(f"<dt><span>{escape(name)} :</span></dt>"
                       f"<dd>{values}</dd>" for name, values in attrs)

    return f"<dl class='xr-attrs'>{attrs_dl}</dl>"


def summarize_attrs(attrs):
    attrs_li = "".join(f"<li class='xr-var-item'>\
            {summarize_variable(name, values, has_attrs=False)}</li>"
                       for name, values in attrs)
    return f"<ul class='xr-var-list'>{attrs_li}</ul>"


def _icon(icon_name):
    # icon_name is defined in icon-svg-inline.html
    return ("<svg class='icon xr-{0}'>"
            "<use xlink:href='#{0}'>"
            "</use>"
            "</svg>".format(icon_name))


def summarize_coord(dim, var):
    is_index = dim in var.dims
    return summarize_variable(str(dim), var, is_index)


def summarize_coords(coords):
    vars_li = "".join(
        f"<li class='xr-var-item'>{summarize_coord(dim, values)}</span></li>"
        for dim, values in coords)

    return f"<ul class='xr-var-list'>{vars_li}</ul>"


def summarize_variable(name, var, is_index=False, has_attrs=False):
    """
    :param name:
    :param var:
    :param is_index: If the variable is an index - used to bold
                     coordinates that represent the indices of
                     a dimension that the data contains

    :param has_attrs: If the variable is for a section that cannot contain
                      attributes, then this hides the show/hide button.
    """
    cssclass_idx = " class='xr-has-index'" if is_index else ""
    dims_text = ', '.join(
        escape(f'{str(dim)} [sparse]' if dim == var.sparse_dim else str(dim))
        for dim in var.dims)
    dims_str = f"({dims_text})"
    name = escape(name)
    dtype = var.dtype
    unit = '' if var.unit == sc.units.dimensionless else var.unit

    # "unique" ids required to expand/collapse subsections
    attrs_id = "attrs-" + str(uuid.uuid4())
    data_id = "data-" + str(uuid.uuid4())
    if hasattr(var, "attrs"):
        disabled = "" if len(var.attrs) > 0 else "disabled"
        attrs_ul = summarize_attrs_simple(var.attrs)
    else:
        disabled = "disabled"
        attrs_ul = None

    preview = inline_variable_repr(var)
    data_repr = f"Values:<br>{short_data_repr_html(var)}"
    variances_preview = ""
    if var.variances is not None:
        variances_preview = inline_variable_repr(var, has_variances=True)
        data_repr += f"<br><br>Variances:<br>\
                       <div>{short_data_repr_html(var)}</div>"

    attrs_icon = _icon("icon-file-text2")
    data_icon = _icon("icon-database")

    html = [
        f"<div class='xr-var-name'><span{cssclass_idx}>{name}</span></div>",
        f"<div class='xr-var-dims'>{dims_str}</div>",
        f"<div class='xr-var-dtype'>{dtype}</div>",
        f"<div class='xr-var-unit'>{unit}</div>",
        f"<div class='xr-value-preview xr-preview'>{preview}</div>",
        f"<div class='xr-variance-preview \
            xr-preview'>{variances_preview}</div>",
        f"<input id='{attrs_id}' class='xr-var-attrs-in' ",
        f"type='checkbox' {disabled}>",
        f"<label for='{attrs_id}' "
        f"class='{'' if has_attrs else 'xr-hide-icon'}'"
        " title='Show/Hide attributes'>",
        f"{attrs_icon}</label>",
        f"<input id='{data_id}' class='xr-var-data-in' type='checkbox'>",
        f"<label for='{data_id}' title='Show/Hide data repr'>",
        f"{data_icon}</label>",
        f"<div class='xr-var-attrs'>{attrs_ul}</div>",
        f"<pre class='xr-var-data'>{data_repr}</pre>",
    ]
    return "".join(html)


def summarize_data(dataset):
    has_attrs = _is_dataset(dataset)
    vars_li = "".join(
        "<li class='xr-var-item'>"
        f"{summarize_variable(name, values, has_attrs=has_attrs)}</li>"
        for name, values in dataset)
    return f"<ul class='xr-var-list'>{vars_li}</ul>"


def collapsible_section(name,
                        inline_details="",
                        details="",
                        n_items=None,
                        enabled=True,
                        collapsed=False,
                        add_value_variance_labels=False,
                        has_attrs=False):
    # "unique" id to expand/collapse the section
    data_id = "section-" + str(uuid.uuid4())

    has_items = n_items is not None and n_items
    n_items_span = "" if n_items is None else f" <span>({n_items})</span>"
    enabled = "" if enabled and has_items else "disabled"
    collapsed = "" if collapsed or not has_items else "checked"
    tip = " title='Expand/collapse section'" if enabled else ""

    if add_value_variance_labels:
        val_var_html = "<div class='sc-section-header "\
            "sc-section-header-values'>"\
            "<span class='sc-section-header-text'>Values</span>"\
            "</div><div class='sc-section-header "\
            "sc-section-header-variances'>"\
            "<span class='sc-section-header-text'>Variances</span>"\
            "</div>"
    else:
        val_var_html = ""

    return (f"<input id='{data_id}' class='xr-section-summary-in' "
            f"type='checkbox' {enabled} {collapsed}>"
            f"<label for='{data_id}' class='xr-section-summary' {tip}>"
            f"{name}:{n_items_span}</label>"
            f"<div class='xr-section-inline-details'>{inline_details}</div>"
            f"{val_var_html}"
            f"<div class='xr-section-details'>{details}</div>")


def _mapping_section(mapping,
                     name,
                     details_func,
                     max_items_collapse,
                     enabled=True,
                     add_value_variance_labels=False):
    n_items = len(mapping)
    collapsed = n_items >= max_items_collapse

    return collapsible_section(
        name,
        details=details_func(mapping),
        n_items=n_items,
        enabled=enabled,
        collapsed=collapsed,
        add_value_variance_labels=add_value_variance_labels,
    )


def dim_section(dataset):
    coords = dataset.coords if hasattr(dataset, "coords") else []
    dim_list = format_dims(dataset.dims, dataset.shape, coords)

    return collapsible_section("Dimensions",
                               inline_details=dim_list,
                               enabled=False,
                               collapsed=True)


def array_section(obj):
    # "unique" id to expand/collapse the section
    data_id = "section-" + str(uuid.uuid4())
    collapsed = ""
    preview = repr(obj.variable)
    data_repr = short_data_repr_html(obj)
    data_icon = _icon("icon-database")

    return ("<div class='xr-array-wrap'>"
            f"<input id='{data_id}' \
            class='xr-array-in' type='checkbox' {collapsed}>"
            f"<label for='{data_id}' \
            title='Show/hide data repr'>{data_icon}</label>"
            f"<div class='xr-array-preview \
            xr-preview'><span>{preview}</span></div>"
            f"<pre class='xr-array-data'>{data_repr}</pre>"
            "</div>")


coord_section = partial(
    _mapping_section,
    name="Coordinates",
    details_func=summarize_coords,
    max_items_collapse=25,
)

label_section = partial(_mapping_section,
                        name="Labels",
                        details_func=summarize_coords,
                        max_items_collapse=10)

mask_section = partial(_mapping_section,
                       name="Masks",
                       details_func=summarize_coords,
                       max_items_collapse=10)

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
    header = f"<div class='xr-header'>"\
        f"{''.join(h for h in header_components)}</div>"
    sections = "".join(f"<li class='xr-section-item'>{s}</li>"
                       for s in sections)

    return ("<div>"
            f"{ICONS_SVG}<style>{CSS_STYLE}</style>"
            "<div class='xr-wrap'>"
            f"{header}"
            f"<ul class='xr-sections'>{sections}</ul>"
            "</div>"
            "</div>")


def dataset_repr(ds):
    obj_type = "scipp.{}".format(type(ds).__name__)

    header_components = [f"<div class='xr-obj-type'>{escape(obj_type)}</div>"]

    sections = [dim_section(ds)]

    # ensure that the values/variances labels are present
    # the first section will add them will also flip this
    # flag so that they are not repeatedly
    add_value_variance_labels = True
    if len(ds.coords) > 0:
        sections.append(
            coord_section(ds.coords,
                          add_value_variance_labels=add_value_variance_labels))
        add_value_variance_labels = False
    if len(ds.labels) > 0:
        sections.append(
            label_section(ds.labels,
                          add_value_variance_labels=add_value_variance_labels))
        add_value_variance_labels = False

    sections.append(
        data_section(ds if hasattr(ds, '__len__') else [('', ds)],
                     add_value_variance_labels=add_value_variance_labels))
    add_value_variance_labels = False

    if len(ds.masks) > 0:
        sections.append(mask_section(ds.masks))
    if len(ds.attrs) > 0:
        sections.append(attr_section(ds.attrs))

    return _obj_repr(header_components, sections)


def variable_repr(var):
    obj_type = "scipp.{}".format(type(var).__name__)

    header_components = [f"<div class='xr-obj-type'>{escape(obj_type)}</div>"]

    sections = [
        dim_section(var),
        data_section([('', var)], add_value_variance_labels=True),
    ]

    return _obj_repr(header_components, sections)
