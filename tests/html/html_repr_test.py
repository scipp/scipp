# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from collections.abc import Callable, Mapping
from functools import reduce
from typing import Any

import hypothesis
import numpy as np
from hypothesis import assume, given
from hypothesis import strategies as st

import scipp as sc
import scipp.testing.strategies as scst

# Tests have an increased deadline because the default was not enough for our MacOS CI.
# Hypothesis failed the tests because of it, but subsequent invocations during
# minimization of the failing example could not reproduce the time-based failure,
# making hypothesis flag the test as flaky.
#
# This might indicate some hidden start-up cost.
# It might be related to complicated utf-8 strings as all failing cases contain many
# unusual characters, many of which cannot be rendered properly on GitHub.


# TODO:
# For now, we are just checking that creating the repr does not throw.


def settings(**kwargs: Any) -> Callable[..., Any]:
    def impl(func: Callable[..., Any]) -> Callable[..., Any]:
        return hypothesis.settings(**{'max_examples': 10, 'deadline': 1000, **kwargs})(
            func
        )

    return impl


@given(var=scst.variables(ndim=0))
@settings()
def test_html_repr_scalar(var: sc.Variable) -> None:
    sc.make_html(var)


@given(var=scst.variables(ndim=st.integers(min_value=1, max_value=4)))
@settings(max_examples=100)
def test_html_repr_variable(var: sc.Variable) -> None:
    sc.make_html(var)
    sc.make_html(var[var.dims[0], 1:10])


def test_html_repr_variable_strings() -> None:
    sc.make_html(sc.array(dims=['x'], values=list(map(chr, range(97, 123)))))


def test_html_repr_variable_vector() -> None:
    sc.make_html(sc.vectors(dims=['x'], values=np.arange(30.0).reshape(10, 3)))


@given(da=scst.dataarrays(data_args={'ndim': 0}))
@settings()
def test_html_repr_data_array_scalar(da: sc.DataArray) -> None:
    da.coords['c'] = sc.array(dims=['w'], values=[4, 5])
    sc.make_html(da)


@given(da=scst.dataarrays(data_args={'ndim': st.integers(min_value=1, max_value=4)}))
@settings()
def test_html_repr_data_array(da: sc.DataArray) -> None:
    sc.make_html(da)
    sc.make_html(da[da.dims[0], 1:10])


@given(da=scst.dataarrays(data_args={'ndim': st.integers(min_value=2, max_value=4)}))
@settings()
def test_html_repr_data_array_nd_coord(da: sc.DataArray) -> None:
    volume = reduce(lambda a, b: a * b, da.shape)
    da.coords['nd'] = sc.arange('aux', volume).fold(dim='aux', sizes=da.sizes)
    sc.make_html(da)


@given(
    buffer=scst.dataarrays(data_args={'ndim': 1}),
    coord_sizes=scst.sizes_dicts(ndim=st.integers(min_value=1, max_value=1)),
)
@settings()
def test_html_repr_binned_data_array(
    buffer: sc.DataArray, coord_sizes: Mapping[str, int]
) -> None:
    dim = next(iter(coord_sizes))
    assume(coord_sizes[dim] > 1)  # need at least length=2 to slice below
    for i, key in enumerate(coord_sizes, 1):
        buffer.coords[key] = i * sc.arange(buffer.dim, len(buffer))
    binned = buffer.bin(coord_sizes)
    sc.make_html(binned)
    sc.make_html(binned[dim, 1:3])
    sc.make_html(binned[dim, 1])
    sc.make_html(binned[dim, 1].bins.data)  # type: ignore[union-attr]


@given(da=scst.dataarrays(data_args={'ndim': st.integers(min_value=1, max_value=4)}))
@settings()
def test_html_repr_dataset(da: sc.DataArray) -> None:
    ds = sc.Dataset({'a': da, 'b': 2 * da})
    sc.make_html(ds)
    sc.make_html(ds[da.dims[0], 1:10])


@given(
    da=scst.dataarrays(data_args={'ndim': st.integers(min_value=1, max_value=4)}),
    var=scst.variables(),
)
@settings()
def test_html_repr_dense_datagroup(da: sc.DataArray, var: sc.Variable) -> None:
    dg = sc.DataGroup(
        {
            'a': da,
            'nested': sc.DataGroup(
                {
                    'b': 2 * da,
                    'vår': var,
                }
            ),
            'number': 5,
        }
    )
    dg_repr_html = sc.make_html(dg)
    from bs4 import BeautifulSoup

    html_parser = BeautifulSoup(dg_repr_html, "html.parser")
    assert type(dg).__name__ in html_parser.find('div', class_='sc-obj-type').text  # type: ignore[union-attr]
    assert bool(html_parser.find('div', class_='dg-root'))
    assert bool(html_parser.find('div', class_='dg-detail-box'))


def test_html_repr_deep_datagroup() -> None:
    dg = sc.DataGroup({'x': sc.scalar(6.2)})
    for i in range(10):
        dg = sc.DataGroup[Any]({f'group-{i}': dg})

    dg_repr_html = sc.make_html(dg)
    from bs4 import BeautifulSoup

    html_parser = BeautifulSoup(dg_repr_html, "html.parser")
    assert type(dg).__name__ in html_parser.find('div', class_='sc-obj-type').text  # type: ignore[union-attr]
    assert bool(html_parser.find('div', class_='dg-root'))
    assert bool(html_parser.find('div', class_='dg-detail-box'))
