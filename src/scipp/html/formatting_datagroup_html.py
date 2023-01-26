# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import uuid
from html import escape
from string import Template
from typing import Union

import numpy as np

from ..core.cpp_classes import DataArray, Dataset, Variable
from ..core.data_group import DataGroup
from ..units import dimensionless
from .resources import load_atomic_tpl, load_collapsible_tpl, load_dg_detail_tpl, \
    load_dg_repr_tpl, load_dg_style


def _make_shape_repr(var: Union[Variable, DataArray, Dataset, DataGroup]) -> str:
    """Returns HTML Component that represents the shape of ``var``"""
    shape_list = "".join(f"<li>{escape(str(dim))}: {size}</li>"
                         for dim, size in var.sizes.items())
    return f"<ul class='sc-dim-list'>{shape_list}</ul>"


def _make_dataset_summary(ds: Dataset) -> str:
    """Returns (partial) information of Dataset object items"""
    key_type_str = [
        ": ".join([name, type(value).__name__]) for name, value in ds.items()
    ]
    if len(key_type_str) > 3:
        key_type_str = key_type_str[:2] + ["..."] + key_type_str[-1:]
    return "Dataset{" + ", ".join(key_type_str) + "}"


def _summarize_atomic_variable(var, name: str, depth: int = 0) -> str:
    """Returns HTML Component that contains summary of ``var``"""
    shape_repr = escape("()")
    unit = ''
    dtype_str = ''
    preview = ''
    parent_obj_str = ''
    objtype_str = type(var).__name__
    if isinstance(var, (Dataset, DataArray, Variable, DataGroup)):
        parent_obj_str = "scipp"
        shape_repr = _make_shape_repr(var)
        if isinstance(var, Dataset):
            preview = _make_dataset_summary(var)
        else:
            if var.unit is not None:
                unit = '𝟙' if var.unit == dimensionless else str(var.unit)
            dtype_str = str(var.dtype)
            preview = str(var.value) if len(var.dims) == 0 else ""
    else:
        if isinstance(var, np.ndarray):
            parent_obj_str = "numpy"
            preview = f"shape={var.shape}, dtype={var.dtype}"
        if isinstance(var, (str, int, float)):
            preview = str(var)

    html_tpl = load_atomic_tpl()
    return Template(html_tpl).substitute(depth=depth,
                                         name=escape(name),
                                         parent=escape(parent_obj_str),
                                         objtype=escape(objtype_str),
                                         shape_repr=shape_repr,
                                         dtype=escape(dtype_str),
                                         unit=escape(unit),
                                         preview=preview)


def _collapsible_summary(var: DataGroup, name: str, name_spaces: list) -> str:
    parent_type = "scipp"
    objtype = type(var).__name__
    shape_repr = _make_shape_repr(var)
    checkbox_id = escape("summary-" + str(uuid.uuid4()))
    depth = len(name_spaces)
    subsection = _datagroup_detail(var, name_spaces + [name])
    html_tpl = load_collapsible_tpl()

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

    dg_detail_tpl = Template(load_dg_detail_tpl())
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
                           shape_repr=_make_shape_repr(dg),
                           details=details)
