# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
import numpy as np
from ...factory import make_dense_data_array, make_dense_dataset


def test_plot_ndarray():
    sc.plot(np.arange(50.))._to_widget()


def test_plot_variable():
    sc.plot(sc.arange('x', 50.))._to_widget()


def test_plot_data_array():
    sc.plot(make_dense_data_array(ndim=1))._to_widget()
    da = make_dense_data_array(ndim=2)
    sc.plot(da)._to_widget()
    da.plot()._to_widget()


def test_plot_data_array_missing_coords():
    sc.data.table_xyz(100).plot()._to_widget()


def test_plot_dataset():
    ds = make_dense_dataset(ndim=1)
    sc.plot(ds)._to_widget()


def test_plot_dict_of_ndarrays():
    sc.plot({'a': np.arange(50.), 'b': np.arange(60.)})._to_widget()


def test_plot_dict_of_variables():
    sc.plot({'a': sc.arange('x', 50.), 'b': sc.arange('x', 60.)})._to_widget()


def test_plot_dict_of_data_arrays():
    ds = make_dense_dataset(ndim=1)
    sc.plot({'a': ds['a'], 'b': ds['b']})._to_widget()
