# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import matplotlib
import matplotlib.pyplot as plt
import pytest
import scipp as sc


@pytest.fixture(autouse=True)
def select_plotting_backend():
    matplotlib.rcdefaults()
    matplotlib.use('Agg')
    sc.plotting.select_backend('experimental')


@pytest.fixture(autouse=True)
def close_figures():
    """
    Force closing all figures after each test case.
    Otherwise, the figures consume a lot of memory and matplotlib complains.
    """
    yield
    for fig in map(plt.figure, plt.get_fignums()):
        plt.close(fig)
