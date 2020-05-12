from typing import List

import bs4

from scipp.table_html.formatting_html import BIN_EDGE_LABEL

VARIABLE_OBJECT_TYPE = "scipp.Variable"
OBJ_TYPE_CSS_CLASS = "xr-obj-type"
VAR_NAME_CSS_CLASS = "sc-var-name"
DATASET_NAME_CSS_CLASS = "xr-var-name"
DTYPE_CSS_CLASS = "xr-var-dtype"
UNIT_CSS_CLASS = "xr-var-unit"
VALUE_CSS_CLASS = "xr-value-preview"
DATA_ICON_CSS_CLASS = "xr-icon-database"
DIMS_CSS_CLASS = "xr-var-dims"
DIM_LIST_CSS_CLASS = "xr-dim-list"

ATTR_NAME = "attr"
LABEL_NAME = "label"
MASK_NAME = "mask"


def assert_obj_type(expected, actual):
    assert len(actual) == 1
    assert expected == actual[0].text


def assert_dims(dims, text, has_events=False, has_bin_edges=False):
    for d in dims:
        assert str(d) in text

    if has_events and not has_bin_edges:
        # only events
        # assert SPARSE_LABEL in text, "Could not find SPARSE"
        assert BIN_EDGE_LABEL not in text, "Unexpected bin-edge label"
    elif not has_events and has_bin_edges:
        # only bin edges
        assert BIN_EDGE_LABEL in text, "Could not find BIN-EDGE"
        # assert SPARSE_LABEL not in text, "Unexpected events label"
    elif has_events and has_bin_edges:
        # both events and bin edges present
        assert BIN_EDGE_LABEL in text, "Could not find BIN-EDGE"
        # assert SPARSE_LABEL in text, "Could not find SPARSE"
    elif not has_events and not has_bin_edges:
        # none present
        assert BIN_EDGE_LABEL not in text, "Unexpected bin-edge label"
        # assert SPARSE_LABEL not in text, "Unexpected events label"


def assert_lengths(lengths, text):
    for length in lengths:
        assert str(length) in text


def assert_dtype(dtype, res_dtype: List[bs4.element.Tag]):
    assert len(res_dtype) == 1
    text = res_dtype[0].text
    # the dtype text is shortened for display
    # so we check if the output is contained in the original str
    assert text in str(dtype)


def assert_unit(unit, res_unit: List[bs4.element.Tag]):
    if unit == "":
        raise RuntimeError("Empty unit string will assert True to everything."
                           "Prefer using an actual unit.")
    assert len(res_unit) == 1
    text = res_unit[0].text
    assert str(unit) in text


def assert_data_repr_icon_present(tags: List[bs4.element.Tag]):
    assert len(tags) == 1


def assert_common(html, in_dtype):
    assert_obj_type(VARIABLE_OBJECT_TYPE,
                    html.find_all(class_=OBJ_TYPE_CSS_CLASS))
    assert_dtype(in_dtype, html.find_all(class_=DTYPE_CSS_CLASS))
    assert_data_repr_icon_present(html.find_all(class_=DATA_ICON_CSS_CLASS))


def assert_dims_section(data, dim_section, events=False):
    dim_list = dim_section.find_all(class_=DIM_LIST_CSS_CLASS)
    assert len(dim_list) == 1

    # checks that all dims are formatted with the their extent
    for dim, size in zip(data.dims, data.shape):
        assert str(dim) in dim_list[0].text
        assert str(size) in dim_list[0].text


def assert_section(section,
                   name,
                   dims,
                   in_dtype,
                   in_unit,
                   has_events=False,
                   has_bin_edges=False):
    if isinstance(name, str):
        return _assert_section_single(section, name, dims, in_dtype, in_unit,
                                      has_events, has_bin_edges)
    elif isinstance(name, list):
        return _assert_section_multiple(section, name, dims, in_dtype, in_unit,
                                        has_events if has_events else [],
                                        has_bin_edges if has_bin_edges else [])


def _assert_section_single(section,
                           name,
                           dims,
                           in_dtype,
                           in_unit,
                           has_events=False,
                           has_bin_edges=False):
    name_html = section.find_all(class_=DATASET_NAME_CSS_CLASS)
    # for checking the names of more than one entry, pass in a list
    assert 0 < len(name_html) < 2, \
        f"Unexpected number of name tags found: {len(name_html)}."\
        "Expected: 1."
    assert str(name) == name_html[0].text

    dims_html = section.find_all(class_=DIMS_CSS_CLASS)
    assert 0 < len(dims_html) < 2, \
        f"Unexpected number of dimension tags found: {len(name_html)}."\
        "Expected: 1."
    assert_dims(dims,
                dims_html[0].text,
                has_events=has_events,
                has_bin_edges=has_bin_edges)
    assert_dtype(in_dtype, section.find_all(class_=DTYPE_CSS_CLASS))
    assert_unit(in_unit, section.find_all(class_=UNIT_CSS_CLASS))
    value = section.find_all(class_=VALUE_CSS_CLASS)
    assert 0 < len(dims_html) < 2, \
        f"Unexpected number of value tags found: {len(name_html)}."\
        "Expected: 1."
    assert "..." in value[0].text


def _assert_section_multiple(section,
                             name,
                             dims,
                             in_dtype,
                             in_unit,
                             has_events=[],
                             has_bin_edges=[]):
    name_html = section.find_all(class_=DATASET_NAME_CSS_CLASS)

    assert len(name_html) == len(name),\
        f"Unexpected number of name tags found: {len(name_html)}."\
        f"Expected: {len(name)}"

    names = [html.text for html in name_html]

    # assert that the name is present in the currently visible variables
    for n in name:
        assert str(n) in names

    dims_html = section.find_all(class_=DIMS_CSS_CLASS)
    for dim, events, bin_edges in zip(dims, has_events, has_bin_edges):
        assert_dims(dim,
                    dims_html[0].text,
                    has_events=events,
                    has_bin_edges=bin_edges)

    for dtype in section.find_all(class_=DTYPE_CSS_CLASS):
        assert_dtype(in_dtype, [dtype])

    for unit in section.find_all(class_=UNIT_CSS_CLASS):
        assert_unit(in_unit, [unit])
