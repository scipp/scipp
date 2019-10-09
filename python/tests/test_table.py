# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sp
from scipp import Dim


def test_dataset_with_1d_data():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64))
    d['Counts'] = sp.Variable([Dim.Tof], values=10.0 * np.random.rand(N))
    sp.table(d)


def test_dataset_with_1d_data_with_bin_edges():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Row] = sp.Variable([Dim.Row],
                                    values=np.arange(N + 1).astype(np.float64))
    d['Counts'] = sp.Variable([Dim.Row], values=10.0 * np.random.rand(N))
    sp.table(d)


def test_dataset_with_1d_data_with_variances():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64))
    d['Counts'] = sp.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    sp.table(d)


def test_dataset_with_1d_data_with_coord_variances():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    variances=0.1 * np.random.rand(N))
    d['Counts'] = sp.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    sp.table(d)


def test_dataset_with_1d_data_with_units():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    unit=sp.units.us,
                                    variances=0.1 * np.random.rand(N))
    d['Sample'] = sp.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              unit=sp.units.m,
                              variances=np.random.rand(N))
    sp.table(d)


def test_dataset_with_0d_data():
    d = sp.Dataset()
    d['Scalar'] = sp.Variable(1.2)
    sp.table(d)


def test_dataset_with_everything():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    unit=sp.units.us,
                                    variances=0.1 * np.random.rand(N))
    d['Counts'] = sp.Variable([Dim.Tof], values=10.0 * np.random.rand(N))
    d['Sample'] = sp.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              unit=sp.units.m,
                              variances=np.random.rand(N))
    d['Scalar'] = sp.Variable(1.2)
    sp.table(d)


def test_variable():
    N = 10
    v = sp.Variable([Dim.Tof],
                    values=np.arange(N).astype(np.float64),
                    unit=sp.units.us,
                    variances=0.1 * np.random.rand(N))
    sp.table(v)


def test_dataset_with_coords_only():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    variances=0.1 * np.random.rand(N))
    sp.table(d)


def test_dataset_with_labels():
    d = sp.Dataset()
    N = 10
    d.coords[Dim.Tof] = sp.Variable([Dim.Tof],
                                    values=np.arange(N).astype(np.float64),
                                    variances=0.1 * np.random.rand(N))
    d['Counts'] = sp.Variable([Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    d.labels["Normalized"] = sp.Variable([Dim.Tof], values=np.arange(N))
    sp.table(d)
