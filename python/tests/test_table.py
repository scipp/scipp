# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sp
import numpy as np
import pytest


def test_dataset_with_1d_data():
    d = sp.Dataset()
    N = 10
    d.set_coord(sp.Dim.Tof,
                sp.Variable([sp.Dim.Tof],
                            values=np.arange(N+1).astype(np.float64)))
    d['Counts'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N))
    html = sp.table(d)


def test_dataset_with_1d_data_with_variances():
    d = sp.Dataset()
    N = 10
    d.set_coord(sp.Dim.Tof,
                sp.Variable([sp.Dim.Tof],
                            values=np.arange(N+1).astype(np.float64)))
    d['Counts'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N),
                              variances=np.random.rand(N))
    html = sp.table(d)


def test_dataset_with_1d_data_with_coord_variances():
    d = sp.Dataset()
    N = 10
    d.set_coord(sp.Dim.Tof,
                sp.Variable([sp.Dim.Tof],
                            values=np.arange(N+1).astype(np.float64),
                            variances=0.1*np.random.rand(N+1)))
    d['Counts'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N),
                              variances=np.random.rand(N))
    html = sp.table(d)


def test_dataset_with_1d_data_with_units():
    d = sp.Dataset()
    N = 10
    d.set_coord(sp.Dim.Tof,
                sp.Variable([sp.Dim.Tof],
                            values=np.arange(N+1).astype(np.float64),
                            unit=sp.units.us,
                            variances=0.1*np.random.rand(N+1)))
    d['Sample'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N),
                              unit=sp.units.m, variances=np.random.rand(N))
    html = sp.table(d)


def test_dataset_with_0d_data():
    d = sp.Dataset()
    N = 10
    d['Scalar'] = sp.Variable(1.2)
    html = sp.table(d)


def test_dataset_with_everything():
    d = sp.Dataset()
    N = 10
    d.set_coord(sp.Dim.Tof,
                sp.Variable([sp.Dim.Tof],
                            values=np.arange(N+1).astype(np.float64),
                            unit=sp.units.us,
                            variances=0.1*np.random.rand(N+1)))
    d['Counts'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N))
    d['Sample'] = sp.Variable([sp.Dim.Tof], values=10.0*np.random.rand(N),
                              unit=sp.units.m, variances=np.random.rand(N))
    d['Scalar'] = sp.Variable(1.2)
    html = sp.table(d)


def test_variable():
    N = 10
    v = sp.Variable([sp.Dim.Tof], values=np.arange(N).astype(np.float64),
                    unit=sp.units.us, variances=0.1*np.random.rand(N))
    sp.table(v)
