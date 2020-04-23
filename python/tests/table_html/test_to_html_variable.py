import numpy as np
import pytest
from bs4 import BeautifulSoup

import scipp as sc
from scipp.table_html import make_html
from scipp.table_html.formatting_html import VARIANCE_PREFIX

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
    assert_common(html, in_dtype)

    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims(dims, name[0].text)
    assert_lengths(lengths, name[0].text)
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1
    assert "..." not in value[0].text


def test_empty_events_1d_variable():
    in_dtype = sc.dtype.event_list_float32
    in_unit = sc.units.K
    var = sc.Variable([], [], unit=in_unit, dtype=in_dtype)

    html = BeautifulSoup(make_html(var), features="html.parser")
    assert_common(html, in_dtype)
    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims([], '')
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1


def test_events_1d_variable():
    in_dtype = sc.dtype.event_list_float32
    in_unit = sc.units.deg
    var = sc.Variable([], [], unit=in_unit, dtype=in_dtype)

    length = 10
    var.values.extend(np.arange(length))

    html = BeautifulSoup(make_html(var), features="html.parser")
    assert_common(html, in_dtype)

    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims([], '')
    # This checks if the 'size' of the Events dim is being added
    # which would add a useless 'None'
    assert "None" not in name[0].text
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1


@pytest.mark.parametrize("dims, lengths",
                         ((['x', 'y'], (10, 3)), (['x', 'y', 'z'],
                                                  (10, 10, 3)),
                          (['x', 'y', 'z', 'spectrum'], (10, 10, 10, 3))))
def test_events(dims, lengths):
    in_dtype = sc.dtype.event_list_float32
    in_unit = sc.units.deg

    var = sc.Variable(dims, lengths, unit=in_unit, dtype=in_dtype)
    length = 10
    var.values[0].extend(np.arange(length))

    html = BeautifulSoup(make_html(var), features="html.parser")
    assert_common(html, in_dtype)

    name = html.find_all(class_=VAR_NAME_CSS_CLASS)
    assert len(name) == 1
    assert_dims(dims, name[0].text)
    assert_unit(in_unit, html.find_all(class_=UNIT_CSS_CLASS))

    value = html.find_all(class_=VALUE_CSS_CLASS)
    assert len(value) == 1
    assert f"len={length}" in value[0].text
