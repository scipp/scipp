# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

# Original source from
# https://github.com/jsignell/xarray/blob/1d960933ab252e0f79f7e050e6c9261d55568057/xarray/core/formatting_html.py

import collections
import operator
import uuid
from collections.abc import Callable, Iterable, Mapping, Sequence
from functools import partial, reduce
from html import escape as html_escape
from re import escape as re_escape
from typing import Any, TypeVar

from .._scipp import core as sc
from ..core import Coords, DataArray, Dataset, Masks, Variable, stddevs
from ..utils import value_to_string
from .resources import load_icons, load_style

BIN_EDGE_LABEL = "[bin-edge]"
STDDEV_PREFIX = "σ = "  # noqa: RUF001
VARIANCES_SYMBOL = "σ²"
SPARSE_PREFIX = "len={}"

_T = TypeVar('_T')


def escape(content: str) -> str:
    """
    Escape dollar-sign($) as well as html special characters.
    Dollar-sign($) without any escape causes unexpected MathJax conversion
    in the Jupyter notebook.
    """
    return html_escape(content).replace('$', re_escape('$'))


def _format_array(data: Sequence[Any], size: int, ellipsis_after: int) -> str:
    i = 0
    s = []
    while i < size:
        if i == ellipsis_after and size > 2 * ellipsis_after + 1:
            s.append("...")
            i = size - ellipsis_after
        elem = data[i]
        if isinstance(elem, DataArray):
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


def _make_row(data_html: object) -> str:
    return f"<div>{data_html}</div>"


def _format_non_events(var: Variable, has_variances: bool) -> str:
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


def _repr_item(bin_dim: int, item: Variable) -> str:
    shape = item.shape[bin_dim]
    return SPARSE_PREFIX.format(shape)


def _get_events(
    var: Variable | DataArray, has_variances: bool, ellipsis_after: int
) -> list[str]:
    constituents = var.bins.constituents
    dim = constituents['dim']
    dims = constituents['data'].dims
    bin_dim = dict(zip(dims, range(len(dims)), strict=True))[dim]
    s = []
    if not isinstance(var.values, Variable | DataArray | Dataset):
        size = len(var.values)
        i = 0

        data = retrieve(var, has_variances=has_variances)
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


def _format_events(var: Variable | DataArray, has_variances: bool) -> str:
    s = _get_events(var, has_variances, ellipsis_after=2)
    return f'binned data [{", ".join(s)}]'


def _ordered_dict(
    data: Mapping[str, _T],
) -> collections.OrderedDict[str, _T]:
    data_ordered = collections.OrderedDict(
        sorted(data.items(), key=lambda t: str(t[0]))
    )
    return data_ordered


def inline_variable_repr(var: Variable | DataArray, has_variances: bool = False) -> str:
    if var.is_binned:
        return _format_events(var, has_variances)
    if isinstance(var, DataArray):
        return _format_non_events(var.data, has_variances)
    else:
        return _format_non_events(var, has_variances)


def retrieve(
    var: Variable | DataArray, has_variances: bool = False, single: bool = False
) -> Any:
    if not has_variances:
        return var.value if single else var.values
    else:
        return var.variance if single else var.variances


def _short_data_repr_html_non_events(
    var: Variable | DataArray, has_variances: bool = False
) -> str:
    return repr(retrieve(var, has_variances))


def _short_data_repr_html_events(var: Variable | DataArray) -> str:
    string = str(var.data) if isinstance(var, DataArray) else str(var)
    if isinstance(var.bins.constituents['data'], Dataset):
        return string
    start = 'binned data: '
    ind = string.find(start) + len(start)
    return string[ind:].replace(', content=', ',\ncontent=')


def short_data_repr_html(var: Variable | DataArray, has_variances: bool = False) -> str:
    """Format "data" for DataArray and Variable."""
    if var.is_binned:
        data_repr = _short_data_repr_html_events(var)
    else:
        data_repr = _short_data_repr_html_non_events(var, has_variances)
    return escape(data_repr)


def format_dims(
    dims: Iterable[str] | None,
    sizes: Iterable[int | None],
    coords: Mapping[str, Variable],
) -> str:
    if dims is None:
        return ""

    dim_css_map = {
        dim: " class='sc-has-index'" if dim in coords else "" for dim in dims
    }

    dims_li = "".join(
        f"<li><span{dim_css_map[dim]}>"
        f"{escape(str(dim))}</span>: "
        f"{size if size is not None else 'Events'}</li>"
        for dim, size in zip(dims, sizes, strict=True)
    )

    return f"<ul class='sc-dim-list'>{dims_li}</ul>"


def _icon(icon_name: str) -> str:
    # icon_name is defined in icon-svg-inline.html
    return (
        f"<svg class='icon sc-{icon_name}'><use xlink:href='#{icon_name}'></use></svg>"
    )


def summarize_coord(
    dim: str, var: Variable, ds: DataArray | Dataset | None = None
) -> str:
    return summarize_variable(dim, var, is_aligned=var.aligned, embedded_in=ds)


def summarize_mask(
    dim: str, var: Variable, ds: DataArray | Dataset | None = None
) -> str:
    return summarize_variable(dim, var, is_aligned=False, embedded_in=ds)


def summarize_coords(coords: Coords, ds: DataArray | Dataset | None = None) -> str:
    vars_li = "".join(
        f"<li class='sc-var-item'>{summarize_coord(dim, var, ds)}</span></li>"
        for dim, var in _ordered_dict(coords).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def summarize_masks(masks: Masks, ds: DataArray | Dataset | None = None) -> str:
    vars_li = "".join(
        f"<li class='sc-var-item'>{summarize_mask(dim, var, ds)}</span></li>"
        for dim, var in _ordered_dict(masks).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def _find_bin_edges(var: Variable | DataArray, ds: DataArray | Dataset) -> list[str]:
    """
    Checks if the coordinate contains bin-edges.
    """
    return [
        dim for dim, length in var.sizes.items() if ds.sizes.get(dim, 1) + 1 == length
    ]


def _make_inline_attributes(
    var: Variable | DataArray, has_attrs: bool
) -> tuple[str, str]:
    disabled = "disabled"
    attrs_ul = ""
    attrs_sections = []

    if has_attrs and hasattr(var, "masks"):
        if len(var.masks) > 0:
            attrs_sections.append(mask_section(var.masks))
            disabled = ""

    if len(attrs_sections) > 0:
        attrs_sections_str = "".join(
            f"<li class='sc-section-item sc-subsection'>{s}</li>"
            for s in attrs_sections
        )
        attrs_ul = (
            "<div class='sc-wrap'>"
            f"<ul class='sc-sections'>{attrs_sections_str}</ul>"
            "</div>"
        )

    return disabled, attrs_ul


def _make_dim_labels(dim: str, bin_edges: Sequence[str] | None = None) -> str:
    # Note: the space needs to be here, otherwise
    # there is a trailing whitespace when no dimension
    # label has been added
    if bin_edges and dim in bin_edges:
        return f" {BIN_EDGE_LABEL}"
    else:
        return ""


def _make_dim_str(
    var: Variable | DataArray,
    bin_edges: Sequence[str] | None,
    add_dim_size: bool = False,
) -> str:
    dims_text = ', '.join(
        '{}{}{}'.format(
            str(dim),
            _make_dim_labels(dim, bin_edges),
            f': {size}' if add_dim_size and size is not None else '',
        )
        for dim, size in zip(var.dims, var.shape, strict=True)
    )
    return dims_text


def _format_common(is_index: bool) -> tuple[str, str, str, str, str]:
    cssclass_aligned = " class='sc-aligned'" if is_index else ""

    # "unique" ids required to expand/collapse subsections
    attrs_id = "attrs-" + str(uuid.uuid4())
    data_id = "data-" + str(uuid.uuid4())
    attrs_icon = _icon("icon-file-text2")
    data_icon = _icon("icon-database")

    return cssclass_aligned, attrs_id, attrs_icon, data_id, data_icon


def summarize_variable(
    name: str | None,
    var: Variable | DataArray,
    is_aligned: bool = False,
    has_attrs: bool = False,
    embedded_in: DataArray | Dataset | None = None,
    add_dim_size: bool = False,
) -> str:
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
    if var.is_binned and isinstance(var.bins.constituents['data'], sc.Dataset):
        # Could print as a tuple of column units and dtypes, but we don't for now.
        unit = ''
        dtype = ''
    elif var.unit is None:
        unit = ''
        dtype = str(var.dtype)
    else:
        unit = '𝟙' if var.unit == sc.units.dimensionless else str(var.unit)  # noqa: RUF001
        dtype = str(var.dtype)

    disabled, attrs_ul = _make_inline_attributes(var, has_attrs)

    preview = _make_row(inline_variable_repr(var))
    data_repr = short_data_repr_html(var)
    if not var.is_binned:
        data_repr = "Values:<br>" + data_repr
    variances_preview = None
    if var.variances is not None:
        variances_preview = _make_row(inline_variable_repr(var, has_variances=True))
        data_repr += f"<br><br>Variances ({VARIANCES_SYMBOL}):<br>\
{short_data_repr_html(var, has_variances=True)}"

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
        f"<div class='sc-var-dtype'>{escape(dtype)}</div>",
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


def summarize_data(dataset: Mapping[str, DataArray] | Dataset) -> str:
    if isinstance(dataset, Dataset):
        has_attrs = True
        embedded_in: Dataset | None = dataset
    else:
        has_attrs = False
        embedded_in = None
    vars_li = "".join(
        "<li class='sc-var-item'>{}</li>".format(
            summarize_variable(
                name,
                var.data,
                has_attrs=has_attrs,
                embedded_in=embedded_in,
            )
        )
        for name, var in _ordered_dict(dataset).items()
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def collapsible_section(
    name: str,
    inline_details: str = "",
    details: str = "",
    n_items: int | None = None,
    enabled: bool = True,
    collapsed: bool = False,
) -> str:
    # "unique" id to expand/collapse the section
    data_id = "section-" + str(uuid.uuid4())

    has_items = n_items is not None and n_items
    n_items_span = "" if n_items is None else f" <span>({n_items})</span>"
    disabled = "" if enabled and has_items else "disabled"
    checked = "" if collapsed or not has_items else "checked"
    tip = " title='Expand/collapse section'" if enabled else ""

    return (
        f"<input id='{data_id}' class='sc-section-summary-in' "
        f"type='checkbox' {disabled} {checked}>"
        f"<label for='{data_id}' class='sc-section-summary' {tip}>"
        f"{name}:{n_items_span}</label>"
        f"<div class='sc-section-inline-details'>{inline_details}</div>"
        f"<div class='sc-section-details'>{details}</div>"
    )


def _mapping_section(
    mapping: DataArray | Mapping[str, Variable] | Dataset | Mapping[str, DataArray],
    *extra_details_func_args: Any,
    name: str,
    max_items_collapse: int,
    details_func: Callable[..., str],
    enabled: bool = True,
) -> str:
    n_items = 1 if isinstance(mapping, DataArray) else len(mapping)
    collapsed = n_items >= max_items_collapse

    return collapsible_section(
        name,
        details=details_func(mapping, *extra_details_func_args),
        n_items=n_items,
        enabled=enabled,
        collapsed=collapsed,
    )


def dim_section(ds: DataArray | Dataset) -> str:
    coords = ds.coords if hasattr(ds, "coords") else {}
    dim_list = format_dims(ds.dims, ds.shape, coords)

    return collapsible_section(
        "Dimensions", inline_details=dim_list, enabled=False, collapsed=True
    )


def summarize_array(var: Variable, is_variable: bool = False) -> str:
    vars_li = "".join(
        "<li class='sc-var-item'>"
        f"{summarize_variable(None, var, add_dim_size=is_variable)}</li>"
    )
    return f"<ul class='sc-var-list'>{vars_li}</ul>"


def variable_section(var: Variable) -> str:
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


def _obj_repr(header_components: Iterable[str], sections: Iterable[str]) -> str:
    header = f"<div class='sc-header'>{''.join(h for h in header_components)}</div>"
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


def _format_size(obj: Variable | DataArray | Dataset) -> str:
    view_size = obj.__sizeof__()
    underlying_size = obj.underlying_size()
    res = f"({human_readable_size(view_size)}"
    if view_size != underlying_size:
        res += (
            " <span class='sc-underlying-size'>out of "
            f"{human_readable_size(underlying_size)}</span>"
        )
    return res + ")"


def data_array_dataset_repr(ds: DataArray | Dataset) -> str:
    obj_type = f"scipp.{type(ds).__name__}"
    header_components = [
        f"<div class='sc-obj-type'>{escape(obj_type)} " + _format_size(ds) + "</div>"
    ]

    sections = [dim_section(ds)]

    if len(ds.coords) > 0:
        sections.append(coord_section(ds.coords, ds))

    sections.append(data_section(ds if isinstance(ds, Dataset) else {'': ds}))

    if not isinstance(ds, Dataset):
        if len(ds.masks) > 0:
            sections.append(mask_section(ds.masks, ds))

    return _obj_repr(header_components, sections)


def variable_repr(var: Variable) -> str:
    obj_type = f"scipp.{type(var).__name__}"

    header_components = [
        f"<div class='sc-obj-type'>{escape(obj_type)} " + _format_size(var) + "</div>"
    ]

    sections = [variable_section(var)]

    return _obj_repr(header_components, sections)


def human_readable_size(size_in_bytes: int) -> str:
    if size_in_bytes / (1024 * 1024 * 1024) > 1:
        return f'{size_in_bytes / (1024 * 1024 * 1024):.2f} GB'
    if size_in_bytes / (1024 * 1024) > 1:
        return f'{size_in_bytes / (1024 * 1024):.2f} MB'
    if size_in_bytes / (1024) > 1:
        return f'{size_in_bytes / (1024):.2f} KB'

    return f'{size_in_bytes} Bytes'
