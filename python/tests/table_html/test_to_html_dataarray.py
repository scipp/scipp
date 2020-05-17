import numpy as np
import pytest
from bs4 import BeautifulSoup

import scipp as sc
from scipp.table_html import make_html

from .common import (ATTR_NAME, LABEL_NAME, MASK_NAME, assert_dims_section,
                     assert_section)


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], (10, )), (['x', 'y'], (10, 10)),
                          (['x', 'y', 'z'], (10, 10, 10)),
                          (['x', 'y', 'z', 'spectrum'], (10, 10, 10, 10))))
def test_basic(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32
    data = sc.Variable(dims,
                       unit=in_unit,
                       dtype=in_dtype,
                       values=np.random.rand(*lengths),
                       variances=np.random.rand(*lengths))

    data_array = sc.DataArray(data,
                              coords={
                                  dims[0]: data,
                                  LABEL_NAME: data
                              },
                              attrs={ATTR_NAME: data},
                              masks={MASK_NAME: data})

    html = BeautifulSoup(make_html(data_array), features="html.parser")
    sections = html.find_all(class_="xr-section-item")
    assert len(sections) == 5
    expected_sections = [
        "Dimensions", "Coordinates", "Data", "Masks", "Attributes"
    ]
    for actual_section, expected_section in zip(sections, expected_sections):
        assert expected_section in actual_section.text

    dim_section = sections.pop(0)
    assert_dims_section(data, dim_section)

    data_names = [
        LABEL_NAME,
        "",  # dataarray does not have a data name
        MASK_NAME,
        ATTR_NAME
    ]
    assert len(sections) == len(
        data_names), "Sections and expected data names do not match"
    for section, name in zip(sections, data_names):
        assert_section(section, name, dims, in_dtype, in_unit)


# @pytest.mark.parametrize("dims, lengths",
#                          ((['x', 'y'], (10, sc.Dimensions.Events)),
#                           (['x', 'y', 'z'], (10, 10, sc.Dimensions.Events)),
#                           (['x', 'y', 'z', 'spectrum'],
#                            (10, 10, 10, sc.Dimensions.Events))))
@pytest.mark.skip(reason="This test currently fails on Windows.")
def test_events(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32

    data = sc.Variable(dims, lengths, unit=in_unit, dtype=in_dtype)

    in_attr_dims = [dims[0]]
    attr = sc.Variable(in_attr_dims,
                       unit=in_unit,
                       dtype=in_dtype,
                       values=np.random.rand(lengths[0]))

    data_array = sc.DataArray(data,
                              coords={
                                  dims[0]: data,
                                  LABEL_NAME: attr
                              },
                              attrs={ATTR_NAME: attr},
                              masks={MASK_NAME: attr})
    html = BeautifulSoup(make_html(data_array), features="html.parser")
    sections = html.find_all(class_="xr-section-item")

    assert len(sections) == 5
    expected_sections = [
        "Dimensions", "Coordinates", "Data", "Masks", "Attributes"
    ]
    for actual_section, expected_section in zip(sections, expected_sections):
        assert expected_section in actual_section.text

    data_section = sections.pop(2)
    assert_section(data_section, "", dims, in_dtype, in_unit, has_events=True)

    dim_section = sections.pop(0)
    assert_dims_section(data, dim_section)

    coord_section = sections.pop(0)
    # the original dim used as a label (dim[0]) is not shown,
    # instead the events dim is shown
    assert_section(coord_section,
                   LABEL_NAME,
                   dims,
                   in_dtype,
                   in_unit,
                   has_events=True)

    data_names = [MASK_NAME, ATTR_NAME]

    for section, name in zip(sections, data_names):
        assert_section(section, name, in_attr_dims, in_dtype, in_unit)


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], [10]), (['x', 'y'], [10, 10]),
                          (['x', 'y', 'z'], [10, 10, 10]),
                          (['x', 'y', 'z', 'spectrum'], [10, 10, 10, 10])))
def test_bin_edge(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32
    data = sc.Variable(dims,
                       unit=in_unit,
                       dtype=in_dtype,
                       values=np.random.rand(*lengths),
                       variances=np.random.rand(*lengths))
    # makes the first dimension be bin-edges
    lengths[0] += 1
    edges = sc.Variable(dims,
                        unit=in_unit,
                        dtype=in_dtype,
                        values=np.random.rand(*lengths))

    data_array = sc.DataArray(data,
                              coords={
                                  dims[0]: edges,
                                  LABEL_NAME: edges
                              },
                              attrs={ATTR_NAME: data},
                              masks={MASK_NAME: edges})

    html = BeautifulSoup(make_html(data_array), features="html.parser")
    sections = html.find_all(class_="xr-section-item")
    assert len(sections) == 5
    expected_sections = [
        "Dimensions", "Coordinates", "Data", "Masks", "Attributes"
    ]
    for actual_section, expected_section in zip(sections, expected_sections):
        assert expected_section in actual_section.text

    attr_section = sections.pop(len(sections) - 1)
    assert_section(attr_section, ATTR_NAME, dims, in_dtype, in_unit)

    data_section = sections.pop(2)
    assert_section(data_section, "", dims, in_dtype, in_unit)

    dim_section = sections.pop(0)
    assert_dims_section(data, dim_section)

    data_names = [LABEL_NAME, MASK_NAME]

    for section, name in zip(sections, data_names):
        assert_section(section,
                       name,
                       dims,
                       in_dtype,
                       in_unit,
                       has_bin_edges=True)


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], [10]), (['x', 'y'], [10, 10]),
                          (['x', 'y', 'z'], [10, 10, 10])))
def test_bin_edge_and_events(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.event_list_float32

    data = sc.Variable(dims=dims, shape=lengths, unit=in_unit, dtype=in_dtype)

    # attribute data without the events dimension
    non_events_data = sc.Variable(dims=dims,
                                  shape=lengths,
                                  unit=in_unit,
                                  dtype=in_dtype)

    # makes the first dimension be bin-edges
    lengths[0] += 1
    non_events_bin_edges = sc.Variable(dims=dims,
                                       shape=lengths,
                                       unit=in_unit,
                                       dtype=in_dtype)

    data_array = sc.DataArray(data,
                              coords={
                                  dims[0]: non_events_bin_edges,
                                  LABEL_NAME: non_events_bin_edges
                              },
                              attrs={ATTR_NAME: non_events_data},
                              masks={MASK_NAME: non_events_bin_edges})

    html = BeautifulSoup(make_html(data_array), features="html.parser")
    sections = html.find_all(class_="xr-section-item")

    expected_sections = [
        "Dimensions", "Coordinates", "Data", "Masks", "Attributes"
    ]
    assert len(sections) == 5
    for actual_section, expected_section in zip(sections, expected_sections):
        assert expected_section in actual_section.text

    attr_section = sections.pop(len(sections) - 1)
    assert_section(attr_section, ATTR_NAME, dims, in_dtype, in_unit)

    data_section = sections.pop(2)
    assert_section(data_section, "", dims, in_dtype, in_unit, has_events=True)

    dim_section = sections.pop(0)
    assert_dims_section(data, dim_section)

    data_names = [LABEL_NAME, MASK_NAME]

    for section, name in zip(sections, data_names):
        assert_section(section,
                       name,
                       dims,
                       in_dtype,
                       in_unit,
                       has_bin_edges=True)


def dataarray_for_repr_test():
    y = sc.Variable(['y'], values=np.arange(2.0), unit=sc.units.m)
    x = sc.Variable(['x'], values=np.arange(3.0), unit=sc.units.m)
    d = sc.DataArray(data=sc.Variable(dims=['y', 'x'],
                                      values=np.random.rand(2, 3)),
                     coords={
                         'yyy': y,
                         'xxx': x,
                         'aux': sc.Variable(['x'], values=np.random.rand(3))
                     },
                     attrs={
                         'attr_zz': x,
                         'attr_aa': y
                     },
                     masks={
                         'mask_zz': x,
                         'mask_aa': y
                     })
    return d


def test_dataarray_repr():
    repr = dataarray_for_repr_test().__repr__()
    # make sure the ordering is correct
    assert repr.find("yyy") > repr.find("xxx") > repr.find("aux")
    assert repr.find('attr_zz') > repr.find('attr_aa')
    assert repr.find('mask_zz') > repr.find('mask_aa')


def test_dataarray_repr_html():
    repr = dataarray_for_repr_test()._repr_html_()
    # make sure the ordering is correct
    assert repr.find("yyy") > repr.find("xxx") > repr.find("aux")
    assert repr.find('attr_zz') > repr.find('attr_aa')
    assert repr.find('mask_zz') > repr.find('mask_aa')
