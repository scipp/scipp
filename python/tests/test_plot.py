# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import mock

import pytest
import scipp as sc

import sys

from scipp.plot import tiled_plot


@pytest.fixture
def patched_plot_collapse():
    plot_collapse_mock = mock.Mock()

    _module = mock.Mock()
    _module.prepare_collapse.return_value = plot_collapse_mock
    sys.modules["scipp.plot.plot_impl.prepare_collapse"] = _module

    yield plot_collapse_mock


@pytest.fixture
def patched_dispatch():
    _module = mock.Mock()
    sys.modules["scipp.plot.plot_impl.dispatch"] = _module

    yield _module.dispatch


def test_plot_throws_for_unknown():
    for i in ["", 1.2, 1, mock.Mock(), None]:
        with pytest.raises(RuntimeError):
            sc.plot.plot(i)


def test_plot_works_with_dataset(patched_dispatch):
    dataset = sc.Dataset()
    dataset["test0"] = sc.Variable([sc.Dim.X], values=[1., 2.])
    dataset["test1"] = sc.Variable([sc.Dim.X], values=[1., 2.])
    sc.plot.plot(dataset)

    patched_dispatch.assert_called()
    assert len(patched_dispatch.call_args_list) == 2, \
        "Expected dispatch to be called twice to do overplotting"
    for i in range(2):
        name, args, kwargs = patched_dispatch.mock_calls[i]
        request = kwargs["request"]

        assert isinstance(request, list)
        assert len(request) == 1
        assert f"test{i}" in request[0].scipp_objs


def test_plot_works_with_variable(patched_dispatch):
    variable = sc.Variable([sc.Dim.X], values=[1., 2.])
    sc.plot.plot(variable)

    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]
    request = kwargs["request"]
    assert str(type(variable)) in request[0].scipp_objs


def test_plot_works_with_data_array(patched_dispatch):
    data_array = sc.DataArray(sc.Variable([sc.Dim.X], values=[1., 2.]))
    sc.plot.plot(data_array)

    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]
    request = kwargs["request"]
    assert isinstance(request, list)
    assert len(request) == 1
    assert data_array.name in request[0].scipp_objs


def test_tiled_plot_throws_for_unrecognised(patched_dispatch):
    inputs = [1.0, sc.Dataset(), sc.Variable, None]

    with pytest.raises(ValueError):
        for i in inputs:
            tiled_plot(i)


def test_tiled_plot_accepts_empty_list_and_set(patched_dispatch):
    dataset = sc.Dataset()

    supported_types = [{dataset}, [dataset]]
    for i in supported_types:
        # We shouldn't throw but nothing to test as dataset is empty
        tiled_plot(i)
        patched_dispatch.assert_not_called()


def test_tiled_plot_single_entry_list_and_set(patched_dispatch):
    variable = sc.Variable([sc.Dim.X], values=[1., 2.])

    supported_types = [{variable}, [variable]]
    for i in supported_types:
        tiled_plot(i)
        patched_dispatch.assert_called_once()
        name, args, kwargs = patched_dispatch.mock_calls[0]

        request = kwargs["request"]
        assert isinstance(request, list)
        assert len(request) == 1

        patched_dispatch.reset_mock()


def test_tiled_plot_with_duplicated_entry_list(patched_dispatch):
    variable = sc.Variable([sc.Dim.X], values=[1., 2.])

    to_plot = [variable, variable, variable]
    tiled_plot(to_plot)
    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]

    request = kwargs["request"]
    assert isinstance(request, list)
    assert len(request) == 3, \
        "Tiled plotting is not handling duplicated names correctly"


def test_tiled_plot_with_mixed_entries(patched_dispatch):
    variable = sc.Variable([sc.Dim.X], values=[1., 2.])
    data_array = sc.DataArray(sc.Variable([sc.Dim.X], values=[1., 2.]))

    inputs = [variable, data_array]
    tiled_plot(inputs)
    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]

    request = kwargs["request"]
    assert isinstance(request, list)
    assert len(request) == 2
