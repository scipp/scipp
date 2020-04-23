# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import mock

import numpy as np
import pytest
import scipp as sc

import sys


@pytest.fixture
def patched_plot_collapse():
    plot_collapse_mock = mock.Mock()

    _module = mock.Mock()
    _module.plot_collapse.return_value = plot_collapse_mock
    sys.modules["scipp.plot.plot_collapse"] = _module

    yield plot_collapse_mock


@pytest.fixture
def patched_dispatch():
    _module = mock.Mock()
    sys.modules["scipp.plot.dispatch"] = _module

    yield _module.dispatch


def test_plot_throws_for_unknown():
    for i in ["", 1.2, 1, mock.Mock(), None]:
        with pytest.raises(RuntimeError):
            sc.plot.plot(i)


def test_plot_works_with_dataset(patched_dispatch):
    dataset = sc.Dataset()
    dataset["test"] = sc.Variable([sc.Dim.X], values=[1., 2.])
    dataset["test2"] = sc.Variable([sc.Dim.X], values=[1., 2.])
    sc.plot.plot(dataset)

    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]
    scipp_objs = kwargs["scipp_obj_dict"]

    assert "test" in scipp_objs
    assert "test2" in scipp_objs


def test_plot_works_with_variable(patched_dispatch):
    variable = sc.Variable([sc.Dim.X], values = [1., 2.])
    sc.plot.plot(variable)

    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]
    scipp_objs = kwargs["scipp_obj_dict"]
    assert str(type(variable)) in scipp_objs


def test_plot_works_with_data_array(patched_dispatch):
    data_array = sc.DataArray(sc.Variable([sc.Dim.X], values=[1., 2.]))
    sc.plot.plot(data_array)

    patched_dispatch.assert_called_once()
    name, args, kwargs = patched_dispatch.mock_calls[0]
    scipp_objs = kwargs["scipp_obj_dict"]
    assert data_array.name in scipp_objs
