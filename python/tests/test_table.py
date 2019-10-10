# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc
from scipp import Dim


def test_dataset_with_1d_data():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Tof] = sc.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64))
    d['Counts'] = sc.Variable([Dim.Tof], values=10.0 * np.random.rand(N))
    sc.table(d)


def test_dataset_with_1d_data_with_bin_edges():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Row] = sc.Variable([Dim.Row],
                                    values=np.arange(N + 1).astype(np.float64))
    d['Counts'] = sc.Variable([Dim.Row], values=10.0 * np.random.rand(N))
    sc.table(d)


def test_dataset_with_1d_data_with_variances():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Tof] = sc.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64))
    d['Counts'] = sc.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    sc.table(d)


def test_dataset_with_1d_data_with_coord_variances():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Tof] = sc.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    variances=0.1 * np.random.rand(N))
    d['Counts'] = sc.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    sc.table(d)


def test_dataset_with_1d_data_with_units():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Tof] = sc.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    unit=sc.units.us,
                                    variances=0.1 * np.random.rand(N))
    d['Sample'] = sc.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              unit=sc.units.m,
                              variances=np.random.rand(N))
    sc.table(d)


def test_dataset_with_0d_data():
    d = sc.Dataset()
    d['Scalar'] = sc.Variable(1.2)
    sc.table(d)


def test_dataset_with_everything():
    d = sc.Dataset()
    N = 10
    d.coords[Dim.Tof] = sc.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    unit=sc.units.us,
                                    variances=0.1 * np.random.rand(N))
    d['Counts'] = sc.Variable([Dim.Tof], values=10.0 * np.random.rand(N))
    d['Sample'] = sc.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              unit=sc.units.m,
                              variances=np.random.rand(N))
    d['Scalar'] = sc.Variable(1.2)
    sc.table(d)


def test_variable():
    N = 10
    v = sc.Variable([Dim.Tof],
                    values=np.arange(N).astype(np.float64),
                    unit=sc.units.us,
                    variances=0.1 * np.random.rand(N))
    sc.table(v)
