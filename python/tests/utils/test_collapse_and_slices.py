# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
import scipp.utils as su



def test_collapse():
    N = 40
    M = 2
    L = 2
    x = np.arange(N).astype(np.float64)
    b = 0.5 * N
    a = 4.0*np.random.random([L, M, N])
    a[1, 1, :] = np.abs(10.0 * np.cos((x-b)*2.0/b))
    d = sc.Dataset()
    d.coords['tof'] = sc.Variable(['tof'], values=x, unit=sc.units.us)
    d.coords['x'] = sc.Variable(['x'], values=np.arange(M).astype(np.float64),
                                   unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=np.arange(L).astype(np.float64),
                                   unit=sc.units.m)
    d['sample'] = sc.Variable(['y', 'x', 'tof'], values=a,
                               variances=0.1*np.random.random([L, M, N]))

    collapsed = sc.collapse(d['sample'], keep='tof')

    # assert collapsed["]
