import numpy as np
import pytest
from bs4 import BeautifulSoup

import scipp as sc
from scipp import Dim
from scipp.table_html import make_html

from .common import (ATTR_NAME, LABEL_NAME, MASK_NAME, assert_dims_section,
                     assert_section)


@pytest.mark.parametrize(
    "dims, lengths", (([Dim.X], [10]), ([Dim.X, Dim.Y], [10, 10]),
                      ([Dim.X, Dim.Y, Dim.Z], [10, 10, 10]),
                      ([Dim.X, Dim.Y, Dim.Z, Dim.Spectrum], [10, 10, 10, 10])))
def test_basic(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32
    data = sc.Variable(dims,
                       unit=in_unit,
                       dtype=in_dtype,
                       values=np.random.rand(*lengths),
                       variances=np.random.rand(*lengths))
    data_name = "testdata"
    bin_edge_data_name = "testdata-binedges"
    lengths[0] += 1
    bin_edges = sc.Variable(dims,
                            unit=in_unit,
                            dtype=in_dtype,
                            values=np.random.rand(*lengths),
                            variances=np.random.rand(*lengths))
    dataset = sc.Dataset({
        data_name: data,
        bin_edge_data_name: bin_edges
    },
                         coords={
                             dims[0]: bin_edges,
                         },
                         attrs={"attr": data},
                         labels={"label": bin_edges},
                         masks={"mask": bin_edges})

    html = BeautifulSoup(make_html(dataset), features="html.parser")
    sections = html.find_all(class_="xr-section-item")

    expected_sections = [
        "Dimensions", "Coordinates", "Labels", "Data", "Masks", "Attributes"
    ]
    assert len(sections) == 6
    for actual_section, expected_section in zip(sections, expected_sections):
        assert expected_section in actual_section.text

    attr_section = sections.pop(len(sections) - 1)
    assert_section(attr_section, ATTR_NAME, dims, in_dtype, in_unit)

    data_section = sections.pop(3)
    assert_section(data_section, [data_name, bin_edge_data_name],
                   dims,
                   in_dtype,
                   in_unit,
                   has_bin_edges=[True, False])

    dim_section = sections.pop(0)
    assert_dims_section(data, dim_section)

    data_names = [dims[0], LABEL_NAME, MASK_NAME]

    for section, name in zip(sections, data_names):
        assert_section(section,
                       name,
                       dims,
                       in_dtype,
                       in_unit,
                       has_bin_edges=True)


def test_sparse_does_not_repeat_dense_coords():
    sparse = sc.Variable([Dim.Y, Dim.Z], shape=(3, sc.Dimensions.Sparse))

    sparse.values[0].extend(np.arange(0))
    sparse.values[2].extend(np.arange(0))
    sparse.values[1].extend(np.arange(0))

    d = sc.Dataset()
    d['a'] = sc.Variable([Dim.Y, Dim.X, Dim.Z],
                         shape=(3, 2, 4),
                         variances=True)
    d['b'] = sc.DataArray(
        sparse,
        coords={
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(4)),
            Dim.Z: sc.Variable([Dim.Z], shape=(sc.Dimensions.Sparse, )),
        },
        labels={"binedge": sc.Variable([Dim.Y], values=np.random.rand(4))},
        attrs={"attr": sc.Variable([Dim.Y], values=np.random.rand(3))})

    html = BeautifulSoup(make_html(d), features="html.parser")
    sections = html.find_all(class_="xr-section-summary")
    assert ["Coordinates" in section.text
            for section in sections].count(True) == 2

    attr_section = next(section for section in sections
                        if "Attributes" in section.text)

    # check that this section is a subsection
    assert "sc-subsection" in attr_section.parent.attrs["class"]

    variables = html.find_all(class_="xr-var-name")

    # check that each dim is only present once
    assert ["z" in var.text for var in variables].count(True) == 1
    assert ["y" in var.text for var in variables].count(True) == 1
