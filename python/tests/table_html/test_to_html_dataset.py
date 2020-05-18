import numpy as np
import pytest
from bs4 import BeautifulSoup

import scipp as sc
from scipp.table_html import make_html

from .common import ATTR_NAME, LABEL_NAME, MASK_NAME,\
                    assert_dims_section, assert_section


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], [10]), (['x', 'y'], [10, 10]),
                          (['x', 'y', 'z'], [10, 10, 10]),
                          (['x', 'y', 'z', 'spectrum'], [10, 10, 10, 10])))
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
                             LABEL_NAME: bin_edges,
                         },
                         attrs={"attr": data},
                         masks={"mask": bin_edges})

    html = BeautifulSoup(make_html(dataset), features="html.parser")
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
    assert_section(data_section, [data_name, bin_edge_data_name],
                   dims,
                   in_dtype,
                   in_unit,
                   has_bin_edges=[True, False])

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


@pytest.mark.skip(reason="This test is currently broken after dims API "
                  "refactor. It gives a Length mismatch on insertion "
                  "error.")
def test_events_does_not_repeat_dense_coords():
    events = sc.Variable(['y', 'z'], shape=(3, sc.Dimensions.Events))

    events.values[0].extend(np.arange(0))
    events.values[2].extend(np.arange(0))
    events.values[1].extend(np.arange(0))

    d = sc.Dataset()
    d['a'] = sc.Variable(['y', 'x', 'z'], shape=(3, 2, 4), variances=True)
    d['b'] = sc.DataArray(
        events,
        coords={
            'y': sc.Variable(['y'], values=np.arange(4)),
            'z': sc.Variable(['z'], shape=(sc.Dimensions.Events, )),
            "binedge": sc.Variable(['y'], values=np.random.rand(4))
        },
        attrs={"attr": sc.Variable(['y'], values=np.random.rand(3))})

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


def dataset_for_repr_test():
    d = sc.Dataset()
    d['z1'] = sc.Variable(['x'], values=[1.0])
    d['a1'] = sc.Variable(['x'], values=[1.0])
    d.attrs['attr1'] = sc.Variable(1.0)
    d.attrs['attr2'] = sc.Variable(1.0)
    d.attrs['attr0'] = sc.Variable(1.0)
    d.masks['zz_mask'] = sc.Variable(dims=['x'], values=np.array([True]))
    d.masks['aa_mask'] = sc.Variable(dims=['x'], values=np.array([True]))
    return d


def test_dataset_repr():
    repr = dataset_for_repr_test().__repr__()
    # make sure the ordering is correct
    assert repr.find("z1") > repr.find("a1")
    assert repr.find("attr2") > repr.find("attr1") > repr.find("attr0")
    assert repr.find("zz_mask") > repr.find("aa_mask")


def test_dataset_repr_html():
    repr = dataset_for_repr_test()._repr_html_()
    # make sure the ordering is correct
    assert repr.find("z1") > repr.find("a1")
    assert repr.find("attr2") > repr.find("attr1") > repr.find("attr0")
    assert repr.find("zz_mask") > repr.find("aa_mask")
