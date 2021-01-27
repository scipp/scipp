import numpy as np
import pytest
from bs4 import BeautifulSoup
import sys

import scipp as sc
from scipp.table_html import make_html
from scipp.table_html.formatting_html import (VARIANCE_PREFIX,
                                              human_readable_size)

from .common import (UNIT_CSS_CLASS, VALUE_CSS_CLASS, VAR_NAME_CSS_CLASS,
                     assert_common, assert_dims, assert_lengths, assert_unit)


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], (10, )), (['x', 'y'], (10, 10)),
                          (['x', 'y', 'z'], (10, 10, 10)),
                          (['x', 'y', 'z', 'spectrum'], (10, 10, 10, 10))))
def test_basic(dims, lengths):
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32

    var = sc.Variable(dims,
                      unit=in_unit,
                      dtype=in_dtype,
                      values=np.random.rand(*lengths),
                      variances=np.random.rand(*lengths))

    html = BeautifulSoup(make_html(var), features="html.parser")

    # Variable's "name" actually contains Dims and their extents
    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims(dims, name[0].text)
    assert_lengths(lengths, name[0].text)
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1

    values, variances = [child.text for child in value[0].children]
    assert "..." in values
    assert VARIANCE_PREFIX in variances


def test_data_not_elided():
    dims = ['x']
    lengths = (3, )
    in_unit = sc.units.m
    in_dtype = sc.dtype.float32

    var = sc.Variable(dims,
                      unit=in_unit,
                      values=np.random.rand(*lengths),
                      dtype=in_dtype)

    html = BeautifulSoup(make_html(var), features="html.parser")
    assert_common(html, in_dtype, sys.getsizeof(var))

    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims(dims, name[0].text)
    assert_lengths(lengths, name[0].text)
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1
    assert "..." not in value[0].text


def test_size_of_unit_conversion():
    assert "4.30 GB" == human_readable_size(4.3 * 1024 * 1024 * 1024)
    assert "4.30 MB" == human_readable_size(4.3 * 1024 * 1024)
    assert "4.30 KB" == human_readable_size(4.3 * 1024)
    assert "5 Bytes" == human_readable_size(5)
