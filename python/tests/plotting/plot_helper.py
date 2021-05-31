# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc


def plot(*args, close=True, **kwargs):
    fig = sc.plot(*args, **kwargs)
    if close:
        fig.close()
