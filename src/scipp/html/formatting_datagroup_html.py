# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import uuid
from string import Template
from typing import Union

import numpy as np

from ..core.cpp_classes import DataArray, Dataset, Variable
from ..core.data_group import DataGroup
from ..units import dimensionless
from .formatting_html import escape
from .resources import load_atomic_row_tpl, load_collapsible_row_tpl, \
    load_dg_detail_list_tpl, load_dg_repr_tpl, load_dg_style


def _merge_wrapped(lines: list, cursor=0) -> list:
    if cursor >= len(lines) - 1:
        return lines
    cur_txt, cur_len = lines[cursor]
    next_txt, next_len = lines[cursor + 1]
    if cur_len + next_len < 15:
        merged_txt = ", ".join([cur_txt, next_txt])
        merged = (merged_txt, len(merged_txt))
        smaller_lines = lines[:cursor] + [merged] + lines[cursor + 2:]
        return _merge_wrapped(smaller_lines, cursor=cursor)
    else:
        return _merge_wrapped(lines, cursor=cursor + 1)


def _merge_lines(lines: list) -> list:
    wrapped = [(repr, len(repr) - repr.count('\\')) for repr in lines]
    merged_wrapped = _merge_wrapped(wrapped)
    return [repr for repr, _ in merged_wrapped]


def _format_shape(var: Union[Variable, DataArray, Dataset, DataGroup],
                  break_lines=True) -> str:
    """Returns HTML Component that represents the shape of ``var``"""
    shape_list = [f"{escape(str(dim))}: {size}" for dim, size in var.sizes.items()]
    if break_lines:
        shape_merged = _merge_lines(shape_list)
        return f"({', <br>&nbsp'.join(shape_merged)})"
    return f"({', '.join(shape_list)})"


def _format_atomic_value(value, maxidx: int = 5) -> str:
    value_repr = str(value)[:maxidx]
    if len(value_repr) < len(str(value)):
        value_repr += "..."
    return value_repr


def _format_dictionary_item(name_item: tuple, maxidx: int = 10) -> str:
    name, item = name_item
    name = _format_atomic_value(name, maxidx=maxidx)
    type_repr = _format_atomic_value(type(item).__name__, maxidx=maxidx)
    return "(" + ": ".join((name, type_repr)) + ")"


def _format_multi_dim_data(var: Union[Variable, DataArray, Dataset, np.ndarray],
                           max_item_number: int = 2) -> str:
    if isinstance(var, Variable):
        if var.ndim != var.values.ndim:
            return _format_atomic_value(var.values, maxidx=None)
        elif var.ndim == 0:
            return _format_atomic_value(var.value, maxidx=30)

    if isinstance(var, Dataset):
        view_iterable = list(var.items())
        format_item = _format_dictionary_item
        var_len = len(var)
    elif isinstance(var, np.ndarray):
        view_iterable = np.ravel(var)
        format_item = _format_atomic_value
        var_len = var.size
    elif isinstance(var, (Variable, DataArray)):
        view_iterable = np.ravel(var.values)
        format_item = _format_atomic_value
        var_len = len(var)

    max_item_number = max(2, max_item_number)
    max_first_items = min(var_len, max_item_number - 1)

    view_iter = iter(view_iterable)
    view_items = [format_item(next(view_iter)) for _ in range(max_first_items)]

    if var_len > max_first_items:
        if var_len > max_item_number:
            view_items.append('... ')
        view_items.append(format_item(view_iterable[-1]))
    return ', '.join(view_items)


def _summarize_atomic_variable(var, name: str, depth: int = 0) -> str:
    """Returns HTML Component that contains summary of ``var``"""
    shape_repr = escape("()")
    unit = ''
    dtype_str = ''
    preview = ''
    parent_obj_str = ''
    objtype_str = type(var).__name__
    if isinstance(var, (Dataset, DataArray, Variable)):
        parent_obj_str = "scipp"
        shape_repr = _format_shape(var)
        preview = _format_multi_dim_data(var)
        if not isinstance(var, Dataset):
            dtype_str = str(var.dtype)
            if var.unit is not None:
                unit = '𝟙' if var.unit == dimensionless else str(var.unit)
    elif isinstance(var, np.ndarray):
        parent_obj_str = "numpy"
        preview = f"shape={var.shape}, dtype={var.dtype}, values="
        preview += _format_multi_dim_data(var)

    elif preview == '' and hasattr(var, "__str__"):
        preview = _format_atomic_value(var, maxidx=30)

    html_tpl = load_atomic_row_tpl()
    return Template(html_tpl).substitute(depth=depth,
                                         name=escape(name),
                                         parent=escape(parent_obj_str),
                                         objtype=escape(objtype_str),
                                         shape_repr=shape_repr,
                                         dtype=escape(dtype_str),
                                         unit=escape(unit),
                                         preview=escape(preview))


def _collapsible_summary(var: DataGroup, name: str, name_spaces: list) -> str:
    parent_type = "scipp"
    objtype = type(var).__name__
    shape_repr = _format_shape(var)
    checkbox_id = escape("summary-" + str(uuid.uuid4()))
    depth = len(name_spaces)
    subsection = _datagroup_detail(var, name_spaces + [name])
    html_tpl = load_collapsible_row_tpl()

    return Template(html_tpl).substitute(name=escape(str(name)),
                                         parent=escape(parent_type),
                                         objtype=escape(objtype),
                                         shape_repr=shape_repr,
                                         summary_section_id=checkbox_id,
                                         depth=depth,
                                         checkbox_status='',
                                         subsection=subsection)


def _datagroup_detail(dg: DataGroup, name_spaces: list = None) -> str:
    if name_spaces is None:
        name_spaces = []
    summary_rows = []
    for name, item in dg.items():
        if isinstance(item, DataGroup):
            collapsible_row = _collapsible_summary(item, name, name_spaces)
            summary_rows.append(collapsible_row)
        else:
            summary_rows.append(
                _summarize_atomic_variable(item, name, depth=len(name_spaces)))

    dg_detail_tpl = Template(load_dg_detail_list_tpl())
    return dg_detail_tpl.substitute(summary_rows=''.join(summary_rows))


def datagroup_repr(dg: DataGroup) -> str:
    """Return HTML Component containing details of ``dg``"""
    obj_type = "scipp.{} ".format(type(dg).__name__)
    checkbox_status = "checked" if len(dg) < 15 else ''
    header_id = "datagroup-view-" + str(uuid.uuid4())
    details = _datagroup_detail(dg)
    html = Template(load_dg_repr_tpl())
    return html.substitute(style_sheet=load_dg_style(),
                           header_id=header_id,
                           checkbox_status=checkbox_status,
                           obj_type=obj_type,
                           shape_repr=_format_shape(dg, break_lines=False),
                           details=details)
